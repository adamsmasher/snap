#ifndef EXPR_H
#define EXPR_H

#include "error.h"

typedef enum {SYMBOL, NUMBER, SUB} Expr_type;

typedef struct Expr_t {
  Expr_type type;
  union {
    int num;
    char* sym;
    struct Expr_t* subexpr[2];
  } e;
} Expr;

Status eval(Expr* e, int* result);

#endif
