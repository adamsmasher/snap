#ifndef EXPR_H
#define EXPR_H

#include "error.h"

typedef enum {SYMBOL, NUMBER, ADD, SUB, STRING_EXPR} Expr_type;

typedef struct Expr_t {
  Expr_type type;
  union {
    int num;
    char* str;
    char* sym;
    struct Expr_t* subexpr[2];
  } e;
} Expr;

Status eval(Expr* e, int* result);

#endif
