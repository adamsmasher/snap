#ifndef EXPR_H
#define EXPR_H

typedef enum {SYMBOL, NUMBER} Expr_type;

typedef struct {
  Expr_type type;
  union {
    int num;
    char* sym;
  } e;
} Expr;

#endif
