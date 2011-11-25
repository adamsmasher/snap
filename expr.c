#include "expr.h"

#include "labels.h"
#include "snap.h"

Status eval(Expr* e, int* result) {
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
  default: return ERROR;
  }
}
