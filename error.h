#ifndef ERROR_H
#define ERROR_H

typedef enum {ERROR, OK} Status;

Status error(const char * format, ...);
Status expected(char e, char c);
Status redefined_label(char* l);

#endif
