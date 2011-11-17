#include "expr.h"

#include "labels.h"

Status eval(Expr* e, int* result) {
  switch(e->type) {
  case NUMBER:
    *result = e->e.num;
    return OK;
  case SYMBOL:
    if(sym_val(e->e.sym, result) != OK)
      return error("undefined symbol '%s'", e->e.sym);
    return OK;
  default: return ERROR;
  }
}
