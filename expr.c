#include "expr.h"

Expr_class expr_class(Expr* e) {
  Expr_class l, r;
  switch(e->type) {
  case SYMBOL: return SYMBOLIC;
  case NUMBER: return NUMERIC;
  case ADD:
  case SUB:
    l = expr_class(e->e.subexpr[0]);
    r = expr_class(e->e.subexpr[0]);
    return l == SYMBOLIC || r == SYMBOLIC ? SYMBOLIC : NUMERIC;
  case STRING_EXPR: return SYMBOLIC;
  }
}
