#include "instructions.h"

#include "error.h"
#include "handlers.h"
#include "table.h"

#include <string.h>
#include <strings.h>

#define INSTRUCTION_BUCKETS 512

int buckets_used = 0;

static void add_handler(char* instruction_name, Handler handler);
int lookup_instruction(char* instruction);

Instruction_entry instruction_table[INSTRUCTION_BUCKETS];

/* this function initializes the instruction handler table with all of the
   assembler's built-in directives and the standard 65816 instructions.
   Currently these are the only things that should *ever* go in the table.
   In the future we may support macros. */
void init_instructions() {
  int i;
  Instruction_entry default_table[] = {
    {"adc", adc},
    {"and", and},
    {"ascii", ascii},
    {"asl", asl},
    {"beq", beq},
    {"bit", bit},
    {"bne", bne},
    {"bpl", bpl},
    {"clc", clc},
    {"db", db},
    {"dw", dw},
    {"dex", dex},
    {"equ", equ},
    {"inc", inc},
    {"inx", inx},
    {"jmp", jmp},
    {"jsr", jsr},
    {"lda", lda},
    {"ldx", ldx},
    {"longa", longa},
    {"longi", longi},
    {"lsr", lsr},
    {"org", org},
    {"pad", pad},
    {"pea", pea},
    {"pha", pha},
    {"pla", pla},
    {"pld", pld},
    {"ply", ply},
    {"rep", rep},
    {"rti", rti},
    {"rtl", rtl},
    {"rts", rts},
    {"sbc", sbc},
    {"sec", sec},
    {"sep", sep},
    {"sta", sta},
    {"stp", stp},
    {"sty", sty},
    {"stz", stz},
    {"tas", tas},
    {"tax", tax},
    {"tcs", tas},
    {"tsa", tsa},
    {"tsc", tsa},
    {"txa", txa},
    {"xce", xce}
  };

  /* set safe dummy values for the table */
  bzero(instruction_table, sizeof(instruction_table));  

  int size = sizeof(default_table)/sizeof(Instruction_entry);
  for(i = 0; i < size; i++)
    add_handler(default_table[i].name, default_table[i].handler);   
}

/* adds a copy of instruction to the handler table, with no handler.
   return a pointer to the copy.
   if instruction is already in the table, do nothing; simply return a pointer
   to the copy.*/
char* intern_instruction(char* instruction) {
  int i = lookup_instruction(instruction);
  if(instruction_table[i].name)
    return instruction_table[i].name;
  else {
    char* name_copy = strdup(instruction);
    instruction_table[i].name = name_copy;
    return name_copy;
  }
}

/* lookup the instruction in the handler table and return the associated
   handler */
Handler get_handler(char* instruction) {
  int i = lookup_instruction(instruction);
  return instruction_table[i].handler;
}

/* adds the instruction_name, handler pair to the table.
   Note that it does not copy instruction_name - do this in advance if you
   want to give the table ownership (i.e. it might be modified or go out of
   scope */
void add_handler(char* instruction_name, Handler handler) {
  int i = lookup_instruction(instruction_name);
  instruction_table[i].name = instruction_name;
  instruction_table[i].handler = handler;
}

/* returns the index that instruction should be in. By 'should', we mean that
   either it's there, or that's where it should be inserted if it's not.
   Simply testing if the slot is empty i.e. if(instruction_table[i].name) will
   tell you what you need to know */
int lookup_instruction(char* instruction) {
  unsigned int i = hash_stri(instruction) % INSTRUCTION_BUCKETS;

  while(instruction_table[i].name && 
        strcasecmp(instruction, instruction_table[i].name))
    i = (i + 1) % INSTRUCTION_BUCKETS;

  return i;
}


