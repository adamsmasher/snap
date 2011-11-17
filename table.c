#include "table.h"

#include <ctype.h>

/* a dumb ol' string hashing function */
unsigned int hash_str(char* str) {
  unsigned int hash = 0;
  for(; *str; str++)
    hash = (hash * 32) - hash + *str;
  return hash;
}    

unsigned int hash_stri(char* str) {
  unsigned int hash = 0;
  for(; *str; str++)
    hash = (hash * 32) - hash + tolower(*str);
  return hash;
}

