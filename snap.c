#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int usage() {
  fprintf(stderr, "Usage: snap <in-file> <out-file>\n");
  return -1;
}

/* defines */
#define LINE_LENGTH 256

/* enums */
typedef enum {IMPLIED,
              IMMEDIATE,
              ABSOLUTE,
              ABSOLUTE_INDEXED_X,
              ABSOLUTE_INDEXED_Y,
              MOVE,
              INDIRECT,
              INDIRECT_INDEXED_Y,
              INDEXED_INDIRECT_X,
              INDIRECT_LONG,
              INDIRECT_LONG_INDEXED_Y,
              STACK_RELATIVE,
              SR_INDIRECT_INDEXED
              } Addressing_mode;
typedef enum {SYMBOL, NUMBER} Expr_type;
typedef enum {ERROR, OK} Status;

/* structures */
typedef struct {
  Expr_type type;
  union {
    int num;
    char* sym;
  } e;
} Expr;

typedef struct Line_tag {
  /* linked list pointer */
  struct Line_tag* next;

  char* label;

  char* instruction;

  /* almost all addressing modes only have one expression - the exception
     being the MVN/MVP one */
  Addressing_mode addr_mode;
  Expr* expr1;
  Expr* expr2;

  /* the assembled machine code for this line */
  int byte_size;
  char bytes[4];
} Line;

/* prototypes */
Status read_file(FILE* fp);
void strip_comment(char* line);
void add_line(Line* line);
char* get_label(char* l, char** label);
char* get_instruction(char* l, char** instruction);
Status get_operand(char* lp, Line* l);
Status read_expr(char** lp, Expr* expr);
Status read_dec(char** lp, int* n);
Status read_hex(char** lp, int* n);
Status read_bin(char** lp, int* n);
Status read_sym(char** lp, char** sym);
void write_assembled(FILE* fp);

int line_num = 0;
Line* first_line = NULL;

/* prints an error message, along with position information, to stderr.
   return ERROR */
Status error(const char * format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, " on line %d\n", line_num);

  va_end(args);
  return ERROR;
}

Status expected(char e, char c) {
  return error("expected '%c', instead found '%c'", e, c);
}

int main(int argc, char** argv) {
  Status status;
  FILE* fp;

  /* we require an infile and an outfile */
  if(argc != 3)
    return usage();

  /* open up the infile and load it into a global list of lines */
  fp = fopen(argv[1], "r");
  if(!fp) {
    fprintf(stderr, "Error: could not open file %s for reading\n", argv[1]);
    return -1;
  }
  status = read_file(fp);
  fclose(fp);
  if(status == ERROR)
    return -1;

  /* assemble it. each line stores its own assembly code */
  status = assemble();
  if(status == ERROR)
    return -1;

  /* write the assembled code out */
  fp = fopen(argv[2], "wb");
  if(!fp) {
    fprintf(stderr, "Error: could not open file %s for writing\n", argv[2]);
    return -1;
  }
  write_assembled(fp);
  fclose(fp);
  return 0;
}

/* allocates and initializes a new Line. returns NULL on failure */
Line* alloc_line() {
  Line* l = malloc(sizeof(Line));
  if(l) {
    l->label = NULL;
    l->instruction = NULL;
    l->byte_size = -1;
  }
  return l;
}

/* reads in a file, loads it into the global line list */
Status read_file(FILE* fp) {
  char l[LINE_LENGTH];

  while(fgets(l, LINE_LENGTH, fp)) {
    char* lp;
    char* label;
    Line* line = alloc_line();
    line_num++;
    strip_comment(l);
    lp = get_label(l, &line->label);
    if(!lp)
      return ERROR;
    lp = get_instruction(lp, &line->instruction);
    if(!lp)
      return ERROR;
    if(line->instruction)
      if(get_operand(lp, line) != OK)
        return ERROR;
    if(line->label || line->instruction)
      add_line(line);
  }
  if(!feof(fp)) {
    fprintf(stderr, "Error: reading from input file\n");
    return ERROR;
  }

  return OK;
}

/* "removes" comments by truncating a string when it detects one */
void strip_comment(char* line) {
  while(*line && *line != ';')
    line++;
  *line = '\n';
  *(line+1) = '\0';
}

void add_line(Line* line) {
  Line* lp;
  if(!first_line)
    first_line = lp;
  else {
    lp = first_line;
    while(lp->next)
      lp = lp->next;
    lp->next = line;
  }
}

/* returns a pointer to the end of the label if there was one,
   otherwise the return value will equal the parameter l, possibly with
   any whitespace skipped. If a label
   is found, copies it and makes label point to it. Returns NULL if
   there was an error. */
char* get_label(char* l, char** label) {
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
    *label[len] = '\0';

    /* return a pointer to after the label */
    l = lp+1;
  }

  return l;
}

/* gets the next word. if it does not exist, it returns l, possibly after
   skipping whitespace.
   if it's an instruction, it makes instruction point to it
   and returns after it. */
char* get_instruction(char* l, char** instruction) {
  char* lp;
 
  /* skip whitespace */
  while(*l && isspace(*l)) l++;

  /* move until the next space */
  lp = l;
  while(*lp && !isspace(lp)) lp++;

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

Status get_operand(char* lp, Line* line) {
  /* skip whitespace */
  while(*lp && isspace(*lp)) lp++;
  
  /* if there's no operand, the operand is implied */
  if(!*lp) {
    line->addr_mode = IMPLIED;
  }
  /* if the operand starts with a #, it's immediate */
  else if(*lp == '#') {
    line->addr_mode = IMMEDIATE;

    /* skip past the # and read the expression */
    lp++;
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;
  }
  /* if the operand starts with a [, it's indirect long */
  else if(*lp == '[') {
    /* skip the '[' and following whitespace */
    lp++;
    while(*lp && isspace(*lp)) lp++;

    /* read in the expression in the []s */
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;

    /* skip whitespace */
    while(*lp && isspace(*lp)) lp++;

    /* expect a ']' */
    if(*lp != ']')
      return expected(']', *lp);
    
    /* skip any whitespace after the ] */
    while(*lp && isspace(*lp)) lp++;

    /* if we're at the end of the line, it's just indirect long */
    if(!*lp)
      line->addr_mode = INDIRECT_LONG;
    /* if there's a comma followed by a 'Y', we're indirect long Y indexed */
    else if(*lp == ',') {
      /* skip whitespace after the comma */
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
    /* skip whitespace after the paren */
    while(*lp && isspace(*lp)) lp++;
    
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

        /* expect a 'Y' */
        if(tolower(*lp) != 'y')
          return expected('Y', *lp);
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
     absolute: lda $01
     indexed: lda $01, X ; lda $01, Y ; lda $01, S
     move: mvn $01, $02 */
  else {
    if(read_expr(&lp, line->expr1) != OK)
      return ERROR;
    /* skip whitespace */
    while(*lp && isspace(*lp)) lp++;
    
    /* if we're at the end of the line, it's absolute */
    if(!*lp)
      line->addr_mode = ABSOLUTE;
    /* if a comma follows the expression, it might be:
     indexed: lda $01, X ; lda $01, Y ; lda $01, S
     move: mvn $01, $02 */
    else if(*lp == ',') {
      /* move past comma */
      lp++;
      while(*lp && isspace(*lp));
    
      switch(tolower(*lp)) {
      case 'x': line->addr_mode = ABSOLUTE_INDEXED_X; lp++; break;
      case 'y': line->addr_mode = ABSOLUTE_INDEXED_Y; lp++; break;
      case 's': line->addr_mode = STACK_RELATIVE; lp++; break;
      default: if(read_expr(&lp, line->expr1) != OK)
                 return ERROR;
               line->addr_mode = MOVE;
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

Status read_expr(char** lp, Expr* expr) {
  if(isdigit(**lp)) {
    expr->type = NUMBER;
    return read_dec(lp, &expr->e.num);
  }
  else if(**lp == '%') {
    expr->type = NUMBER;
    (*lp)++;
    return read_bin(lp, &expr->e.num);
  }
  else if(**lp == '$') {
    expr->type = NUMBER;
    (*lp)++;
    return read_hex(lp, &expr->e.num);
  }
  else {
    expr->type = SYMBOL;
    return read_sym(lp, &expr->e.sym);
  }
}

Status read_dec(char** lp, int* n) {
  while(**lp && !isspace(**lp)) {
    if(!isdigit(**lp))
      return error("unexpected '%c' in numerical constant", **lp);
    *n = *n * 10 + (**lp - '0');
    (*lp)++;
  }
  return OK;
}

Status read_hex(char** lp, int* n) {
  while(**lp && !isspace(**lp)) {
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

Status read_bin(char** lp, int* n) {
  while(**lp && !isspace(**lp)) {
    if(**lp != '0' && **lp != '1')
      return error("unexpected '%c' in numerical constant", **lp);
    *n = *n * 2 + (**lp - '0');
    (*lp)++;
  }
  return OK;
}

Status read_sym(char** lp, char** sym) {
  char backup;
  char* lp2 = *lp;
  while(*lp2 && isalnum(*lp2) || *lp2 == '_' || *lp2 == '.') lp2++;
  backup = *lp2;
  *lp2 = '\0';
  *sym = intern_symbol(*lp);
  *lp2 = backup;
  *lp = lp2;
  return OK;
}

/* iterates through the assembled lines, writing each to disk */
void write_assembled(FILE* fp) {
  Line* lp = first_line;
  while(lp) {
    fwrite(lp->bytes, 1, lp->byte_size, fp);
    lp = lp->next;
  }
}
