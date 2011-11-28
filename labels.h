#ifndef LABELS_H
#define LABELS_H

#include "error.h"

void init_symtable();
char* intern_symbol(char* sym);
Status set_val(char* sym, int val);
Status sym_val(char* sym, int* dest);
void kill_locals();

#endif
