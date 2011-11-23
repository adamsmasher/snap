#include "lines.h"

#include <stdlib.h>

Line* first_line = NULL;

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
  Line* lp;
  if(!first_line)
    first_line = line;
  else {
    lp = first_line;
    while(lp->next)
      lp = lp->next;
    lp->next = line;
    line->next = NULL;
  }
}

