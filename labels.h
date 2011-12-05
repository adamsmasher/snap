#ifndef LABELS_H
#define LABELS_H

#include "error.h"

#include <stdio.h>

void init_symtable();
char* intern_symbol(char* sym);
Status set_val(char* sym, int val);
Status sym_val(char* sym, int* dest);
void dump_symbols(FILE* fp);

#endif
