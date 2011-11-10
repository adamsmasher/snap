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

typedef enum {IMMEDIATE, ABSOLUTE, MOVE} Addressing_mode;

typedef enum {SYMBOL, NUMBER} Expr_type;

typedef enum {ERROR, OK} Status;

/* prints an error message, along with position information, to stderr.
   return ERROR */
Status error(const char * format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, " on line %d\n", line_num);

  va_end(args)
  return ERROR;
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
    l->operand = NULL;
    l->byte_size = -1;
  }
  return l;
}

/* reads in a file, loads it into the global line list */
Status read_file(FILE* fp) {
  char l[LINE_LENGTH];

  while(read_line(fp, l)) {
    char* lp;
    char* label;
    Line* l = alloc_line();
    line_num++;
    strip_comment(l);
    lp = get_label(l, &line->label);
    if(!lp)
      return ERROR;
    lp = get_instruction(lp, &line->instruction);
    if(!lp)
      return ERROR;
    if(line->instruction)
      lp = get_operand(lp, line);
    if(line->label || line->instruction)
      add_line(line);
  }
}

/* "removes" comments by truncating a string when it detects one */
void strip_comment(char* line) {
  while(*line && line != ';')
    line++;
  *line = '\n';
  *(line+1) = '\0';
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

char* get_operand(char* lp, Line* l) {
  /* skip whitespace */
  while(*lp && isspace(*lp)) lp++;
  
  /* if the operand starts with a #, it's immediate */
  if(*lp == '#') {
    line->addr_mode = IMMEDIATE;
    line->expr1 = read_atom(*lp);
  }
  /* if the operand starts with a [, it's indirect long '['? */
  else if(*lp == '[') {
    /* skip whitespace */

    line->expr1 = read_num(&lp);

    /* skip whitespace */

    /* expect a ']' */

    /* if we're at the end of the line, it's just indirect long */

    /* if there's a comma followed by a 'Y', we're indirect long Y indexed */

    /* anything else is an error */
  }
  /* if the operand starts with a ( it's indirect */
  else if(*lp == '(') {
    
  }
  /* otherwise it's an absolute, possibly indexed, or a move */
  else {
    line->expr1 = read_atom(&lp);
    /* skip whitespace */
    
    /* if we're at the end of the line, it's absolute */

    /* if there's a comma followed by an X, Y, or S it's indexed */

    /* if there's a comma followed by anything else, it's a move */

    /* anything else is an error */
  }
}  

/* if the operand starts with a digit, then it starts with a number.
     %, $, and - can also prefix numbers. The first two specify binary
     and hex, respectively, and a - of course is unary minus */
  else if(isdigit(*lp) || *lp == '%' || *lp == '$') {
    line->expr1 = read_num(&lp);
    
    /* skip whitespace */
    while(*lp && isspace(*lp)) lp++;

    /* we might have a comma */
    if(*lp == ',') {
      /* skip whitespace */
      
      /* another expression means move semantics */

      /* an X means absolute indexed, X */

      /* a Y means absolute indexed, Y */

      /* an S means stack relative */

      /* anything else is invalid */
      
    }
    
    /* if we're at the end of the line, we've got an absolute. */

    /* anything else is unexpected and an error */
  }


/* iterates through the assembled lines, writing each to disk */
void write_assembled(FILE* fp) {
  Line* lp = first_line;
  while(*lp) {
    fwrite(lp->bytes, 1, lp->byte_size, fp);
    lp = lp->next;
  }
}
