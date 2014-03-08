#ifndef EXPR_H
#define EXPR_H

typedef enum {SYMBOL, NUMBER, ADD, SUB, STRING_EXPR} Expr_type;
typedef enum {NUMERIC, SYMBOLIC} Expr_class;

typedef struct Expr_t {
  Expr_type type;
  union {
    int num;
    char* str;
    char* sym;
    struct Expr_t* subexpr[2];
  } e;
  struct Expr_t* next; /* for a list */
} Expr;

Expr_class expr_class(Expr* expr);

#endif
