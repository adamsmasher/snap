#ifndef ERROR_H
#define ERROR_H

#include "lines.h"

typedef enum {ERROR, OK} Status;

Status error(const char * format, ...);
Status expected(char e, char c);
Status invalid_operand(Line* l);
Status redefined_label(char* l);

#endif
