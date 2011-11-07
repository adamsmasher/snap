typedef struct {
  char* label;
  char* instruction;
  char* operand;
} Line;

typedef enum {} Operand_type;

typedef enum {ERROR, OK} Status;

/* prints an error message, along with position information, to stderr.
   return ERROR */
Status error(const char * format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, " on line %d\n", line_num);

  return ERROR;
}

int main(int argc, char** argv) {
  Status status;
  FILE* fp;
  if(argc != 3)
    return usage();
  fp = fopen(argv[1], "r");
  if(!fp) {
    fprintf(stderr, "Error: could not open file %s for reading\n", argv[1]);
    return -1;
  }
  status = read_file(fp);
  fclose(fp);
  if(status == ERROR)
    return -1;
  status = assemble();
  if(status == ERROR)
    return -1;
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
    lp = get_instruction(lp, &line->instruction);
    if(!lp)
      return ERROR;
    if(line->instruction)
      lp = get_operand(lp, &line->operand);
    if(!eol(lp))
      return error("Unexpected %s", lp);
    if(line->label || line->instruction)
      add_line(line);
  }
}

/* "removes" comments by truncating a string when it detects one */
void strip_comment(char* line) {
  while(*line && line != ';')
    line++;
  *(line-1) = '\n';
  *line = '\0';
}
