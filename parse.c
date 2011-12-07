#include "parse.h"

#include "error.h"
#include "instructions.h"
#include "labels.h"
#include "lines.h"
#include "snap.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_LENGTH 256

static Status incsrc(Line* line);
static void strip_comment(char* line);
static char* get_label(char* l, char** label);
static char* get_instruction(char* l, char** instruction);
static Status get_operand(char* lp, Line* l);
static Status read_list(char** lp, Line* line);
static Status read_expr(char** lp, Expr* expr);
static Status read_atom(char** lp, Expr* expr);
static Status read_dec(char** lp, int* n);
static Status read_hex(char** lp, int* n);
static Status read_bin(char** lp, int* n);
static Status read_sym(char** lp, char** sym);
static Status read_str(char** lp, char** str);

/* reads in and parses a file, loads it into the global line list */
Status read_file(FILE* fp) {
  char l[LINE_LENGTH];

  line_num = 0;
  while(fgets(l, LINE_LENGTH, fp)) {
    char* lp;
    Line* line = alloc_line();

    /* bookkeeping */
    line_num++;
    line->line_num = line_num;
    line->filename = current_filename;

    strip_comment(l);

    lp = get_label(l, &line->label);
    if(!lp)
      return ERROR;

    lp = get_instruction(lp, &line->instruction);
    if(!lp)
      return ERROR;

    if(line->label && line->label[0] != '.' &&
       (!line->instruction || strcasecmp(line->instruction, "equ") != 0)) {
      current_label = line->label;
      current_label_len = strlen(current_label);
    }

    if(line->instruction) {
      if(get_operand(lp, line) != OK)
        return ERROR;
    }

    /* special case for incsrc */
    if(line->instruction && strcasecmp(line->instruction, "incsrc") == 0) {
      line->instruction = NULL;
      if(line->label)
        add_line(line);
      if(incsrc(line) != OK)
        return ERROR;
    }
    else if(line->label || line->instruction)
      add_line(line);

    if(!line->instruction && !line->label)
      free(line);
  }
  if(!feof(fp)) {
    fprintf(stderr, "Error: reading from input file\n");
    return ERROR;
  }

  return OK;
}

Status incsrc(Line* line) {
  char* backup_filename;
  int backup_linenum;
  if(line->addr_mode != STRING || line->expr1->type != STRING_EXPR)
    return invalid_operand(line);

  backup_filename = current_filename;
  backup_linenum = line_num;
  current_filename = line->expr1->e.str;

  if(load_file(current_filename) != OK)
    return ERROR;

  current_filename = backup_filename;
  line_num = backup_linenum;

  return OK;
}

/* "removes" comments by truncating a string when it detects one */
static void strip_comment(char* line) {
  while(*line && *line != ';')
    line++;
  *line = '\n';
  *(line+1) = '\0';
}

/* returns a pointer to the end of the label if there was one,
   otherwise the return value will equal the parameter l, possibly with
   any whitespace skipped. If a label
   is found, copies it and makes label point to it. Returns NULL if
   there was an error. */
static char* get_label(char* l, char** label) {
  char* lp;

  /* skip whitespace */
  while(isspace(*l)) l++;

  /* labels can't start with digits, it makes them easier to detect while
     lexing */
  if(isdigit(*l)) {
    error("label or instruction cannot begin with digit");
    return NULL;
  }

  /* advance to the end of the label */
  lp = l;
  while(*lp && !isspace(*lp) && *lp != ':') lp++;

  /* did we find a label? */
  if(*lp == ':') {
    int len;

    /* get length. if 0, then the line started with a ':', which is bad */
    len = lp - l;
    if(!len) {
      error("no label name specified");
      return NULL;
    }

    /* copy the label into fresh storage */
    *label = malloc(len + 1);
    memcpy(*label, l, len);
    (*label)[len] = '\0';

    /* return a pointer to after the label */
    l = lp+1;
  }

  return l;
}

/* gets the next word. if it does not exist, it returns l, possibly after
   skipping whitespace.
   if it's an instruction, it makes instruction point to it
   and returns after it. */
static char* get_instruction(char* l, char** instruction) {
  char* lp;
 
  /* skip whitespace */
  while(*l && isspace(*l)) l++;

  /* move until the next space */
  lp = l;
  while(*lp && !isspace(*lp)) lp++;

  /* if we moved ahead, there's an instruction there...*/
  if(l != lp) {
    /* turn the string into a null-terminated one */
    char backup = *lp;
    *lp = '\0';

    *instruction = intern_instruction(l);

    /* restore the next character */
    *lp = backup;
  }
  return lp;
}

static Status get_operand(char* lp, Line* line) {
  /* skip whitespace */
  while(*lp && isspace(*lp)) lp++;
  
  /* if there's no operand, the operand is implied */
  if(!*lp) {
    line->addr_mode = IMPLIED;
  }
  else if(*lp == '"') {
    line->addr_mode = STRING;
    line->expr1 = malloc(sizeof(Expr));
    line->expr1->type = STRING_EXPR;
    if(read_str(&lp, &line->expr1->e.str) != OK)
      return ERROR;
  }
  /* if the operand starts with a #, it's immediate */
  else if(*lp == '#') {
    line->addr_mode = IMMEDIATE;

    lp++;
    if(*lp == '^') {
      line->modifier = IMMEDIATE_HI;
      lp++;
    }
    else if(*lp == '>') {
      line->modifier = IMMEDIATE_MID;
      lp++;
    }
    else if(*lp == '<') {
      line->modifier = IMMEDIATE_LO;
      lp++;
    }

    line->expr1 = malloc(sizeof(Expr));
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;
  }
  /* if the operand starts with a [, it's indirect long */
  else if(*lp == '[') {
    /* skip the '[' and following whitespace */
    lp++;
    while(*lp && isspace(*lp)) lp++;

    /* read in the expression in the []s */
    line->expr1 = malloc(sizeof(Expr));
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;

    /* skip whitespace */
    while(*lp && isspace(*lp)) lp++;

    /* expect a ']' */
    if(*lp != ']')
      return expected(']', *lp);
    
    /* skip any whitespace after the ] */
    lp++;
    while(*lp && isspace(*lp)) lp++;

    /* if we're at the end of the line, it's just indirect long */
    if(!*lp)
      line->addr_mode = INDIRECT_LONG;
    /* if there's a comma followed by a 'Y', we're indirect long Y indexed */
    else if(*lp == ',') {
      /* skip whitespace after the comma */
      lp++;
      while(*lp && isspace(*lp)) lp++;
      /* expect Y */
      if(tolower(*lp) != 'y')
        return expected('Y', *lp);
      line->addr_mode = INDIRECT_LONG_INDEXED_Y;
      /* move past the Y */
      lp++;
    }
  }
  /* if the operand starts with a ( it's going to be:
    indirect:            lda ( $01 )
    indirect indexed y:  lda ( $01 ) , Y
    indexed indirect x:  lda ( $01 ,  X )
    SR indirect indexed: lda ( $01 , S ) , Y */
  else if(*lp == '(') {
    lp++;
    /* skip whitespace after the paren */
    while(*lp && isspace(*lp)) lp++;
    
    line->expr1 = malloc(sizeof(Expr));
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;

    /* skip whitespace after the expr */
    while(*lp && isspace(*lp)) lp++;

    /* if a rparen immediately follows the expression, we might have:
      indirect: lda ($01)
      indirect indexed y: lda ($01), Y */
    if(*lp == ')') {
      lp++;
      while(*lp && isspace(*lp)) lp++;

      /* are we at the end of the line? if so, we have:
        indirect: lda ($01) */
      if(!*lp)
        line->addr_mode = INDIRECT;
      /* is there a comma followed by a Y? if so, we have:
        indirect indexed y: lda ($01), Y */
      else if(*lp == ',') {
        /* skip whitespace after the comma */
        lp++;
        while(*lp && isspace(*lp)) lp++;
        
        /* expect a Y */
        if(tolower(*lp) != 'y')
          return expected('Y', *lp);
        /* skip past the Y */
        lp++;
        line->addr_mode = INDIRECT_INDEXED_Y;
      }
      /* anything else following the ) is unexpected, will be caught at the
         end of this function */
    }
    /* if a comma follows the expression, we might have:
      indexed indirect: lda ($01, X)
      SR indirect indexed: lda ($01, S), Y */
    else if(*lp == ',') {
      /* move past the comma */
      lp++;
      while(*lp && isspace(*lp)) lp++;

      /* we're expecting either X or S */
      if(!*lp)
        return error("unexpected end-of-line");
      /* if the comma is followed by X, we have:
        indexed indirect: lda ( $01 , X ) */
      else if(tolower(*lp) == 'x') {
        /* move past the X */
        lp++;
        while(*lp && isspace(*lp)) lp++;
        
        /* expect a ')' */
        if(*lp != ')')
          return expected(')', *lp);
        /* move past the ')' */
        lp++;
        line->addr_mode = INDEXED_INDIRECT_X;
      }
      /* if the comma is followed by S, we have:
        SR indirect indexed: lda ( $01 , S ) , Y */
      else if(tolower(*lp) == 's') {
        /* move past the S */
        lp++;
        while(*lp && isspace(*lp)) lp++;

        /* expect a ')' */
        if(*lp != ')')
          return expected(')', *lp);
        /* move past the ')' */
        lp++;
        while(*lp && isspace(*lp)) lp++;

        /* expect a ',' */
        if(*lp != ',')
          return expected(',', *lp);

        /* move past the ',' */
        lp++;
        while(*lp && isspace(*lp)) lp++;

        /* expect a 'Y' */
        if(tolower(*lp) != 'y')
          return expected('Y', *lp);
        lp++;
        line->addr_mode = SR_INDIRECT_INDEXED;
      }
      /* anything else after the , is unexpected and will be caught at the
         end of this function */
    }
    /* if we don't have anything, error */
    else if(!*lp) {
      return error("unexpected end-of-line");
    }
    /* anything else following the expression is unexpected, will be handled at
       the end of this function*/
  }
  /* otherwise it's:
     accumulator: lda A
     absolute: lda $01
     indexed: lda $01, X ; lda $01, Y ; lda $01, S
     move: mvn $01, $02 */
  else {
    if(tolower(*lp) == 'a') {
      char* lookahead = lp;
      lookahead++;
      while(*lookahead && isspace(*lookahead)) lookahead++;
      if(!*lookahead) {
        line->addr_mode = ACCUMULATOR;
        lp = lookahead;
        return OK;
      }
    }
    line->expr1 = malloc(sizeof(Expr));
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;
    /* skip whitespace */
    while(*lp && isspace(*lp)) lp++;

    /* if we're at the end of the line, it's absolute */
    if(!*lp)
      line->addr_mode = ABSOLUTE;
    /* if a comma follows the expression, it might be:
     indexed: lda $01, X ; lda $01, Y ; lda $01, S
     list: db 0, 1, 2, 3... */
    else if(*lp == ',') {
      /* move past comma */
      lp++;
      while(*lp && isspace(*lp)) lp++;
 
      switch(tolower(*lp)) {
      case 'x': line->addr_mode = ABSOLUTE_INDEXED_X; lp++; break;
      case 'y': line->addr_mode = ABSOLUTE_INDEXED_Y; lp++; break;
      case 's': line->addr_mode = STACK_RELATIVE; lp++; break;
      default: 
        if(read_list(&lp, line) != OK)
          return ERROR;
      }
    }
  /* anything else following the expression is unexpected, will be handled
     at the end of this function */
  }
  /* clear any extraneous whitespace */
  while(*lp && isspace(*lp)) lp++;

  if(*lp)
    return error("unexpected %s", lp);
  return OK;
}  

/* helper function to read the tail of a list.
   Assumes the head of the list is in line->expr1 
   transforms the line into a LIST line */
static Status read_list(char** lp, Line* line) {
  int list_size = 1;
  Expr* end;  

  line->addr_mode = LIST;
  line->expr2 = end = line->expr1;
  while(**lp) {
    list_size++;
    end->next = malloc(sizeof(Expr));
    end = end->next;
    if(read_expr(lp, end) != OK)
      return ERROR;
    /* skip whitespace */
    while(**lp && isspace(**lp)) (*lp)++;

    /* expect either a comma or nothing */
    if(**lp) {
      if(**lp != ',')
        return expected(',', **lp);
      (*lp)++;
      while(**lp && isspace(**lp)) (*lp)++;
    }
  }
  end->next = NULL;

  line->expr1 = malloc(sizeof(Expr));
  line->expr1->type = NUMBER;
  line->expr1->e.num = list_size;

  return OK;
}

static Status read_expr(char** lp, Expr* expr) {
  Expr* l;
  l = malloc(sizeof(Expr));
  if(read_atom(lp, l) != OK)
    return ERROR;

  while(**lp && isspace(**lp)) (*lp)++;

  if(**lp == '-' || **lp == '+') {
    expr->e.subexpr[0] = l;
    expr->e.subexpr[1] = malloc(sizeof(Expr));
    if(**lp == '-')
      expr->type = SUB;
    else if(**lp == '+')
      expr->type = ADD;
    (*lp)++;
    while(**lp && isspace(**lp)) (*lp)++;
    return read_expr(lp, expr->e.subexpr[1]);
  }
  else {
    *expr = *l;
    return OK;
  }
}

static Status read_atom(char** lp, Expr* expr) {
  if(isdigit(**lp) || **lp == '%' || **lp == '$') {
    int l;
    if(isdigit(**lp)) {
      if(read_dec(lp, &l) != OK)
         return ERROR;
    }
    else if(**lp == '%') {
      (*lp)++;
      if(read_bin(lp, &l) != OK)
        return ERROR;
    }
    else if(**lp == '$') {
      (*lp)++;
      if(read_hex(lp, &l) != OK)
        return ERROR;
    }
    expr->type = NUMBER;
    expr->e.num = l;
    return OK;
  }
  else {
    char* l;
    if(read_sym(lp, &l) != OK)
      return ERROR;
    expr->type = SYMBOL;
    expr->e.sym = l;
    return OK;
  }
}

static int numsep(char c) {
  return isspace(c) || c == ',' || c == '-' || c == '+' || c == ']' || c == ')';
}

static Status read_dec(char** lp, int* n) {
  *n = 0;
  while(**lp && !numsep(**lp)) {
    if(!isdigit(**lp))
      return error("unexpected '%c' in numerical constant", **lp);
    *n = *n * 10 + (**lp - '0');
    (*lp)++;
  }
  return OK;
}

static Status read_hex(char** lp, int* n) {
  *n = 0;
  while(**lp && !numsep(**lp)) {
    if(!isxdigit(**lp))
      return error("unexpected '%c' in numerical constant", **lp);
    if(isdigit(**lp))
      *n = *n * 16 + (**lp - '0');
    else /* is A-F */
      *n = *n * 16 + (10 + tolower(**lp) - 'a');
    (*lp)++;
  }
  return OK;
}

static Status read_bin(char** lp, int* n) {
  *n = 0;
  while(**lp && !numsep(**lp)) {
    if(**lp != '0' && **lp != '1')
      return error("unexpected '%c' in numerical constant", **lp);
    *n = *n * 2 + (**lp - '0');
    (*lp)++;
  }
  return OK;
}

static Status read_sym(char** lp, char** sym) {
  char backup;
  char* lp2 = *lp;
  while(*lp2 && (isalnum(*lp2) || *lp2 == '_' || *lp2 == '.')) lp2++;
  backup = *lp2;
  *lp2 = '\0';
  *sym = intern_symbol(*lp);
  *lp2 = backup;
  *lp = lp2;
  return OK;
}

static Status read_str(char** lp, char** str) {
  int len;
  char* lp2;

  /* skip the first " */
  (*lp)++;
  lp2 = *lp;

  /* reach the end of the string */
  while(*lp2 && *lp2 != '"') {
    if(*lp2 == '\\') 
      lp2++;
    lp2++;
  }
  if(!*lp2)
    return error("unterminated string constant");

  len = lp2 - *lp;
  *str = malloc(len + 1);
  memcpy(*str, *lp, len);
  (*str)[len] = '\0';

  *lp = lp2;
  (*lp)++;

  return OK;
}
