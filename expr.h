#ifndef EXPR_H
#define EXPR_H

#include "error.h"

typedef enum {SYMBOL, NUMBER} Expr_type;

typedef struct {
  Expr_type type;
  union {
    int num;
    char* sym;
  } e;
} Expr;

Status eval(Expr* e, int* result);

#endif
