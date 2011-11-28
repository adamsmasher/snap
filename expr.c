#include "expr.h"

#include "labels.h"
#include "snap.h"

Status eval(Expr* e, int* result) {
  int l;
  int r;

  if(!e)
    return OK;

  switch(e->type) {
  case NUMBER:
    *result = e->e.num;
    return OK;
  case SYMBOL:
    if(sym_val(e->e.sym, result) != OK) {
      missing_labels = 1;
      if(pass) 
          return error("undefined symbol '%s'", e->e.sym);
      else
          return ERROR;
    }
    return OK;
  case ADD:
  case SUB:
    if(eval(e->e.subexpr[0], &l) != OK)
      return ERROR;
    if(eval(e->e.subexpr[1], &r) != OK)
      return ERROR;
    if(e->type == ADD)
      *result = l + r; 
    else if(e->type == SUB)
      *result = l - r;
    return OK;
  default: return ERROR;
  }
}
