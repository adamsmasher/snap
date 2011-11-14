#include "snap.h"

#include "error.h"
#include "lines.h"

#include <stdio.h>
#include <stdlib.h>

int usage() {
  fprintf(stderr, "Usage: snap <in-file> <out-file>\n");
  return -1;
}

/* globals */
int line_num = 0;

/* prototypes */
Status assemble();
void write_assembled(FILE* fp);

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

Status assemble() {
  Line* lp = first_line;
  while(lp) {
    lp = lp->next;
  }
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
