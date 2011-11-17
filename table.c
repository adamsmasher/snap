#include "table.h"

/* a dumb ol' string hashing function */
unsigned int hash_str(char* str) {
  unsigned int hash = 0;
  for(; *str; str++)
    hash = (hash * 32) - hash + *str;
  return hash;
}    

