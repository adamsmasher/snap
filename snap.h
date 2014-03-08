#ifndef SNAP_H
#define SNAP_H

#include "error.h"

extern int acc16;
extern char* current_filename;
extern char* current_label;
extern int current_label_len;
extern int index16;
extern int line_num;
extern int missing_labels;
extern int pass;
extern int pc;
extern int d;
extern int dbr;

Status load_file(char* filename);

#endif
