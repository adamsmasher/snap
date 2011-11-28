#include "error.h"

#include "lines.h"
#include "snap.h"

#include <stdarg.h>
#include <stdio.h>

/* prints an error message, along with position information, to stderr.
   return ERROR */
Status error(const char * format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "%s: ", current_filename ? current_filename : "Error");
  vfprintf(stderr, format, args);
  fprintf(stderr, " on line %d\n", line_num);

  va_end(args);
  return ERROR;
}

Status expected(char e, char c) {
  return error("expected '%c', instead found '%c'", e, c);
}

Status redefined_label(char* l) {
  return error("redefinition of label %s", l);
}

Status invalid_operand(Line* line) {
  return error("invalid operand");
}

