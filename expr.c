#include "expr.h"

#include "labels.h"

Status eval(Expr* e, int* result) {
  switch(e->type) {
  case NUMBER:
    *result = e->e.num;
    return OK;
  case SYMBOL:
    return sym_val(e->e.sym, result);
  default: return ERROR;
  }
}
