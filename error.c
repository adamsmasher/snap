#include "error.h"

#include "snap.h"

#include <stdarg.h>
#include <stdio.h>

/* prints an error message, along with position information, to stderr.
   return ERROR */
Status error(const char * format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, " on line %d\n", line_num);

  va_end(args);
  return ERROR;
}

Status expected(char e, char c) {
  return error("expected '%c', instead found '%c'", e, c);
}

