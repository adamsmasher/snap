#ifndef LABELS_H
#define LABELS_H

#include "error.h"

void init_symtable();
char* intern_symbol(char* sym);
Status sym_val(char* sym, int* dest);

#endif
