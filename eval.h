#ifndef EVAL_H
#define EVAL_H

#include "error.h"
#include "expr.h"

Status eval(Expr* e, int* result);

#endif
