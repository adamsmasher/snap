#ifndef LINES_H
#define LINES_H

#include "expr.h"

typedef enum {ACCUMULATOR,
              IMPLIED,
              IMMEDIATE,
              ABSOLUTE,
              ABSOLUTE_INDEXED_X,
              ABSOLUTE_INDEXED_Y,
              INDIRECT,
              INDIRECT_INDEXED_Y,
              INDEXED_INDIRECT_X,
              INDIRECT_LONG,
              INDIRECT_LONG_INDEXED_Y,
              STACK_RELATIVE,
              SR_INDIRECT_INDEXED,
              STRING,
              LIST
} Addressing_mode;

typedef enum {NONE,
              IMMEDIATE_HI,
              IMMEDIATE_MID,
              IMMEDIATE_LO
} Addressing_modifier;

typedef struct Line_tag {
  /* linked list pointer */
  struct Line_tag* next;

  char* filename;
  int line_num;

  char* label;

  char* instruction;

  /* almost all addressing modes only have one expression - the exception
     being the MVN/MVP one */
  Addressing_mode addr_mode;
  Addressing_modifier modifier;
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
