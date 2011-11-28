#ifndef SNAP_H
#define SNAP_H

#include "error.h"

extern int acc16;
extern char* current_filename;
extern int index16;
extern int line_num;
extern int missing_labels;
extern int pass;
extern int pc;

Status load_file(char* filename);

#endif
