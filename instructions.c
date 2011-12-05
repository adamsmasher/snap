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
    {"bcc", bcc},
    {"bcs", bcs},
    {"beq", beq},
    {"bge", bcs},
    {"bgt", bcs},
    {"bit", bit},
    {"blt", bcc},
    {"bmi", bmi},
    {"bne", bne},
    {"bpl", bpl},
    {"bra", bra},
    {"brk", brk},
    {"brl", brl},
    {"bvc", bvc},
    {"bvs", bvs},
    {"clc", clc},
    {"cld", cld},
    {"cli", cli},
    {"clv", clv},
    {"cmp", cmp},
    {"cop", cop},
    {"cpx", cpx},
    {"cpy", cpy},
    {"db", db},
    {"dw", dw},
    {"dec", dec},
    {"dex", dex},
    {"dey", dey},
    {"eor", eor},
    {"equ", equ},
    {"inc", inc},
    {"incbin", incbin},
    {"inx", inx},
    {"iny", iny},
    {"jml", jml},
    {"jmp", jmp},
    {"jsl", jsl},
    {"jsr", jsr},
    {"lda", lda},
    {"ldx", ldx},
    {"ldy", ldy},
    {"longa", longa},
    {"longi", longi},
    {"lsr", lsr},
    {"mvn", mvn},
    {"mvp", mvp},
    {"nop", nop},
    {"ora", ora},
    {"org", org},
    {"pad", pad},
    {"pea", pea},
    {"pei", pei},
    {"per", per},
    {"pha", pha},
    {"phb", phb},
    {"phd", phd},
    {"phk", phk},
    {"php", php},
    {"phx", phx},
    {"phy", phy},
    {"pla", pla},
    {"plb", plb},
    {"pld", pld},
    {"plp", plp},
    {"plx", plx},
    {"ply", ply},
    {"rep", rep},
    {"rol", rol},
    {"ror", ror},
    {"rti", rti},
    {"rtl", rtl},
    {"rts", rts},
    {"sbc", sbc},
    {"sec", sec},
    {"sed", sed},
    {"sei", sei},
    {"sep", sep},
    {"sta", sta},
    {"stp", stp},
    {"stx", stx},
    {"sty", sty},
    {"stz", stz},
    {"swa", xba},
    {"tad", tad},
    {"tas", tas},
    {"tax", tax},
    {"tay", tay},
    {"tcd", tad},
    {"tcs", tas},
    {"tda", tda},
    {"tdc", tda},
    {"trb", trb},
    {"tsa", tsa},
    {"tsb", tsb},
    {"tsc", tsa},
    {"tsx", tsx},
    {"txa", txa},
    {"txs", txs},
    {"txy", txy},
    {"tya", tya},
    {"tyx", tyx},
    {"wai", wai},
    {"xba", xba},
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


