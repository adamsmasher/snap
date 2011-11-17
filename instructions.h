#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "error.h"
#include "lines.h"

typedef Status (*Handler)(Line* line);

typedef struct {
  char* name;
  Handler handler;
} Instruction_entry;

void init_instructions();
char* intern_instruction(char* instruction);
Handler get_handler(char* instruction);

#endif
