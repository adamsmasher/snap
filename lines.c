#include "lines.h"

#include <stdlib.h>

Line* first_line = NULL;
Line* last_line = NULL;

/* allocates and initializes a new Line. returns NULL on failure */
Line* alloc_line() {
  Line* l = malloc(sizeof(Line));
  if(l) {
    l->label = NULL;
    l->instruction = NULL;
    l->byte_size = 0;
    l->expr1 = NULL;
    l->expr2 = NULL;
  }
  return l;
}

void add_line(Line* line) {
  if(!first_line) 
    first_line = last_line = line;
  else {
    last_line->next = line;
    line->next = NULL;
    last_line = line;
  }
}

