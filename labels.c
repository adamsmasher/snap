#include "labels.h"

#include "error.h"
#include "snap.h"
#include "table.h"

#include <stdlib.h>
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
static char* add_namespace(char* sym);

Status sym_val(char* sym, int* dest) {
  int i = lookup_symbol(sym);

  if(symbol_table[i].name && symbol_table[i].defined) {
    *dest = symbol_table[i].val;
    return OK;
  }
  else return ERROR;
}

Status set_val(char* sym, int val) {
  int i = lookup_symbol(sym);

  if(symbol_table[i].name && symbol_table[i].defined == pass + 1)
    return redefined_label(sym);
  else {
    intern_symbol(sym);
    symbol_table[i].defined = pass + 1;
    symbol_table[i].val = val;
    return OK;
  }
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
    char* name_copy;
    if(sym[0] == '.')
      name_copy = add_namespace(sym);
    else
      name_copy = strdup(sym);
    symbol_table[i].name = name_copy;
    return name_copy;
  }
}

int lookup_symbol(char* sym) {
  char* name_copy;
  int i;

  if(sym[0] == '.')
    name_copy = add_namespace(sym);
  else
    name_copy = strdup(sym);

  i = hash_str(name_copy) % SYMBOL_BUCKETS;
  while(symbol_table[i].name && 
        strcmp(name_copy, symbol_table[i].name))
    i = (i + 1) % SYMBOL_BUCKETS;

  free(name_copy);

  return i;
}

/* if we have a local label like .loop, and a global label like Function,
   return Function:loop.
   The user cannot accidentally create this label because : is forbidden in
   labels */
char* add_namespace(char* sym) {
  char* copy;
  int len = strlen(sym) + current_label_len;
  
  copy = malloc(len + 1);
  memcpy(copy, current_label, len);
  copy[current_label_len] = ':';
  strcpy(&copy[current_label_len+1], &sym[1]); 

  return copy;
}
