#ifndef LINES_H
#define LINES_H

#include "expr.h"

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

typedef struct Line_tag {
  /* linked list pointer */
  struct Line_tag* next;

  int line_num;

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

extern Line* first_line;

Line* alloc_line();
void add_line(Line* line);

#endif
