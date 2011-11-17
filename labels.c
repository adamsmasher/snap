#include "labels.h"

#include "error.h"
#include "table.h"

#include <string.h>
#include <strings.h>

#define SYMBOL_BUCKETS 256

typedef struct {
  char* name;
  int val;
  int defined;
} Symbol_entry;

Symbol_entry symbol_table[SYMBOL_BUCKETS];

static int lookup_symbol(char* sym);

Status sym_val(char* sym, int* dest) {
  int i = lookup_symbol(sym);

  if(symbol_table[i].name && symbol_table[i].defined) {
    *dest = symbol_table[i].val;
    return OK;
  }
  else return ERROR;
}

void init_symtable() {
  bzero(symbol_table, sizeof(symbol_table));
}

/* adds a copy of sym to the symbol table, with no value.
   return a pointer to the copy.
   if symbol is already in the table, do nothing; simply return a pointer
   to the copy.*/
char* intern_symbol(char* sym) {
  int i = lookup_symbol(sym);
  if(symbol_table[i].name)
    return symbol_table[i].name;
  else {
    char* name_copy = strdup(sym);
    symbol_table[i].name = name_copy;
    return name_copy;
  }
}

int lookup_symbol(char* sym) {
  int i = hash_str(sym) % SYMBOL_BUCKETS;

  while(symbol_table[i].name && 
        strcmp(sym, symbol_table[i].name))
    i = (i + 1) % SYMBOL_BUCKETS;

  return i;
}

