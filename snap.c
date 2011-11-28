#include "snap.h"

#include "error.h"
#include "instructions.h"
#include "labels.h"
#include "lines.h"
#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int usage() {
  fprintf(stderr, "Usage: snap <in-file> <out-file>\n");
  return -1;
}

/* globals */
int acc16 = 0;
char* current_label = "";
int current_label_len = 0;
char* current_filename = NULL;
int index16 = 0;
int line_num = 0;
int missing_labels = 0;
int pass = 0;
int pc = 0;

/* prototypes */
Status assemble();
void write_assembled(FILE* fp);

int main(int argc, char** argv) {
  FILE* fp;

  /* we require an infile and an outfile */
  if(argc != 3)
    return usage();

  /* initialization */
  init_instructions();
  init_symtable();

  if(load_file(argv[1]) != OK)
    return -1;

  /* assemble it. each line stores its own assembly code */
  if(assemble() != OK)
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

Status load_file(char* filename) {
  FILE* fp;
  Status status;

   /* open up the infile and load it into a global list of lines */
  fp = fopen(filename, "r");
  if(!fp) {
    fprintf(stderr, "Error: could not open file %s for reading\n", filename);
    return ERROR;
  }
  current_filename = filename;
  status = read_file(fp);
  fclose(fp);
  return status;
}

Status assemble() {
  Handler f;
  Line* lp;
  int old_byte_size;
  int another_pass;

  /* continue doing passes until we can't make any more optimizations */
  do {
    another_pass = 0;
    /* continue doing passes as long as we're missing labels */
    do {
      lp = first_line;
      acc16 = index16 = 0;
      missing_labels = 0;
      current_label = "";
      current_label_len = 0;
      while(lp) {
        line_num = lp->line_num;
        current_filename = lp->filename;
        old_byte_size = lp->byte_size;
        /* add the label */
        if(lp->label) {
          /* special case for constants */
          if(!lp->instruction || strcasecmp(lp->instruction, "equ") != 0) {
            if(set_val(lp->label, pc) != OK)
              return ERROR;
            if(lp->label[0] != '.') {
              current_label = lp->label;
              current_label_len = strlen(current_label);
            }
          }
        }
        /* assemble the instruction */
        if(lp->instruction) {
          /* lookup the handler for the instruction */
          if(!(f = get_handler(lp->instruction)))
            return error("unknown instruction '%s'", lp->instruction);
          if(f(lp) != OK)
            return ERROR; 
          pc += lp->byte_size;
          if(old_byte_size > lp->byte_size) {
            printf("debug: line %d was %db, now %db.\n", line_num, old_byte_size, lp->byte_size);
            another_pass = 1;
          }
        }
        lp = lp->next;
      }
      pass++;
    } while(missing_labels);
  } while(another_pass);
  return OK;
}

/* iterates through the assembled lines, writing each to disk */
void write_assembled(FILE* fp) {
  Line* lp = first_line;
  while(lp) {
    if(lp->instruction && strcasecmp(lp->instruction, "pad") == 0)
      while(lp->byte_size) {
        fputc(0, fp);
        lp->byte_size--;
      }
    else if(lp->instruction && strcasecmp(lp->instruction, "ascii") == 0)
      fwrite(lp->expr1->e.str, 1, lp->byte_size, fp);
    else
      fwrite(lp->bytes, 1, lp->byte_size, fp);
    lp = lp->next;
  }
}
