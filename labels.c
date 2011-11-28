#include "labels.h"

#include "error.h"
#include "snap.h"
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
Symbol_entry locals[SYMBOL_BUCKETS];

static int lookup_symbol(char* sym, Symbol_entry* table);

Status sym_val(char* sym, int* dest) {
  Symbol_entry* table;
  int i;

  table = sym[0] == '.' ? locals : symbol_table;
  i = lookup_symbol(sym, table);

  if(table[i].name && table[i].defined) {
    *dest = table[i].val;
    return OK;
  }
  else return ERROR;
}

Status set_val(char* sym, int val) {
  Symbol_entry* table;
  int i;

  table = sym[0] == '.' ? locals : symbol_table;
  i = lookup_symbol(sym, table);

  if(table[i].name && table[i].defined == pass + 1)
    return redefined_label(sym);
  else {
    intern_symbol(sym);
    table[i].defined = pass + 1;
    table[i].val = val;
    return OK;
  }
}

void init_symtable() {
  bzero(symbol_table, sizeof(symbol_table));
  bzero(locals, sizeof(locals));
}

void kill_locals() {
  bzero(locals, sizeof(locals));
}

/* adds a copy of sym to the symbol table, with no value.
   return a pointer to the copy.
   if symbol is already in the table, do nothing; simply return a pointer
   to the copy.*/
char* intern_symbol(char* sym) {
  int i;
  Symbol_entry* table;

  table = sym[0] == '.' ? locals : symbol_table;
  i = lookup_symbol(sym, table);
  if(table[i].name)
    return table[i].name;
  else {
    char* name_copy = strdup(sym);
    table[i].name = name_copy;
    return name_copy;
  }
}

int lookup_symbol(char* sym, Symbol_entry* table) {
  int i = hash_str(sym) % SYMBOL_BUCKETS;

  while(table[i].name && 
        strcmp(sym, table[i].name))
    i = (i + 1) % SYMBOL_BUCKETS;

  return i;
}



