#include "handlers.h"

#include "expr.h"
#include "labels.h"
#include "lines.h"
#include "snap.h"

#include <strings.h>

#define PRIMARY_IMM 0x09
#define PRIMARY_DP 0x05
#define PRIMARY_ABS 0x0D
#define PRIMARY_ABS_LONG 0x0F
#define PRIMARY_ABS_INDEXED_X 0x1D
#define PRIMARY_ABS_LONG_INDEXED_X 0x1F

#define ADC_BASE 0x60

#define AND_BASE 0x20

#define ASL_ACC 0x0A
#define ASL_DP 0x06
#define ASL_ABS 0x0E

#define BEQ 0xF0

#define BIT_IMM 0x89
#define BIT_DP 0x24
#define BIT_ABS 0x2C

#define BNE 0xD0

#define CLC 0x18

#define DEX 0xCA

#define INC_ACC 0x1A
#define INC_DP 0xE6
#define INC_ABS 0xEE

#define INX 0xE8

#define JMP_ABS                  0x4C
#define JMP_ABS_INDEXED_INDIRECT 0x7C

#define JSR_ABS                  0x20
#define JSR_ABS_INDEXED_INDIRECT 0xFC

#define LDA_BASE 0xA0

#define LDX_IMM 0xA2
#define LDX_DP 0xA6
#define LDX_ABS 0xAE

#define LSR_ACC 0x4A
#define LSR_ABS 0x4E
#define LSR_DP 0x46

#define PEA 0xF4

#define PHA 0x48

#define PLA 0x68

#define PLD 0x2B

#define PLY 0x7A

#define REP 0xC2

#define RTL 0x6B

#define RTS 0x60

#define SBC_BASE 0xE0

#define SEC 0x38

#define SEP 0xE2

#define STA_DP 0x85
#define STA_ABS 0x8D
#define STA_ABS_LONG 0x8F
#define STA_ABS_INDEXED_X 0x9D
#define STA_ABS_LONG_INDEXED_X 0x9F

#define STZ_ABS 0x9C
#define STZ_DP 0x64

#define TAS 0x1B

#define TAX 0xAA

#define TSA 0x3B

#define TXA 0x8A

#define XCE 0xFB

#define LO(x) ((char)(x))
#define MID(x) ((char)((x) >> 8))
#define HI(x) ((char)((x) >> 16))

Status invalid_operand(Line* line) {
  return error("invalid operand");
}

Status operand_too_large(int operand) {
  return error("operand %d too large", operand);
}

Status branch_out_of_bounds(Line* line) {
  if(line->expr1->type == SYMBOL)
    return error("destination %s must be within 128 bytes of branch",
                 line->expr1->e.sym);  
  else
    return error("destination must be within 128 bytes of branch");
}

Status jump_out_of_bounds(Line* line) {
  if(line->expr1->type == SYMBOL)
    return error("destination %s must be within same bank as jump",
                 line->expr1->e.sym);  
  else
    return error("destination must be within same bank as jump");
}


static Status primary(Line* line, int base, int sixteen_bit) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      /* set byte_size, assuming the worst */
      switch(line->addr_mode) {
      case IMMEDIATE:
        line->byte_size = sixteen_bit ? 3 : 2;
        return OK;
      case ABSOLUTE:
      case ABSOLUTE_INDEXED_X:
        line->byte_size = 4;
        return OK;
      default: return invalid_operand(line);
      }
    }
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    line->byte_size = acc16 ? 3 : 2;
    line->bytes[0] = base + PRIMARY_IMM;
    line->bytes[1] = LO(operand);
    if(sixteen_bit) line->bytes[2] = MID(operand);
    else if(operand > 0xFF)
      return operand_too_large(operand);
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = base + PRIMARY_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = base + PRIMARY_ABS;
    }
    else if(operand <= 0xFFFFFF) {
      line->byte_size = 4;
      line->bytes[0] = base + PRIMARY_ABS_LONG;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    line->bytes[3] = HI(operand);
    break;
  case ABSOLUTE_INDEXED_X:
    if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = base + PRIMARY_ABS_INDEXED_X;
    }
    else if(operand <= 0xFFFFFF) {
      line->byte_size = 4;
      line->bytes[0] = base + PRIMARY_ABS_LONG_INDEXED_X;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    line->bytes[3] = HI(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

static Status implicit(Line* line, int op) {
  switch(line->addr_mode) {
  case IMPLIED:
    line->byte_size = 1;
    line->bytes[0] = op;
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status adc(Line* line) {
  return primary(line, ADC_BASE, acc16);
}

Status and(Line* line) {
  return primary(line, AND_BASE, acc16);
}

Status asl(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      line->byte_size = 3;
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ACCUMULATOR:
    line->byte_size = 1;
    line->bytes[0] = ASL_ACC;
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = ASL_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = ASL_ABS;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status beq(Line* line) {
  int operand;
  char dest;

  line->byte_size = 2;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand - pc >= 128 || operand - pc < -128)
      return branch_out_of_bounds(line);
    dest = (char)(operand - pc);
    line->bytes[0] = BEQ;
    line->bytes[1] = dest;
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status bit(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case IMMEDIATE: 
        line->byte_size = acc16 ? 3 : 2;
        break;
      case ABSOLUTE:
        line->byte_size = 3;
        break;
      default: return invalid_operand(line);
      }
      return OK;
    }
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    line->byte_size = acc16 ? 3 : 2;
    line->bytes[0] = BIT_IMM;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = BIT_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = BIT_ABS;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}


Status bne(Line* line) {
  int operand;
  char dest;

  line->byte_size = 2;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else { 
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand - pc >= 128 || operand - pc < -128)
      return branch_out_of_bounds(line);
    dest = (char)(operand - pc);
    line->bytes[0] = BNE;
    line->bytes[1] = dest;
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status clc(Line* line) { return implicit(line, CLC); }
Status dex(Line* line) { return implicit(line, DEX); }

Status equ(Line* line) {
  int operand;

  line->byte_size = 0;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  if(line->addr_mode == ABSOLUTE) {
    if(!line->label)
      return error("Must specify label for EQU");
    else
      return set_val(line->label, operand);
  }
  else
    return invalid_operand(line);
}

Status inc(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      line->byte_size = 3;
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ACCUMULATOR:
    line->byte_size = 1;
    line->bytes[0] = INC_ACC;
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = INC_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = INC_ABS;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status inx(Line* line) { return implicit(line, INX); }

Status jmp(Line* line) {
  int operand;

  line->byte_size = 3;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(HI(operand) != HI(pc))
      return jump_out_of_bounds(line);
    line->bytes[0] = JMP_ABS;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}


Status jsr(Line* line) {
  int operand;

  line->byte_size = 3;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(HI(operand) != HI(pc))
      return jump_out_of_bounds(line);
    line->bytes[0] = JSR_ABS;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}


Status lda(Line* line) {
  return primary(line, LDA_BASE, acc16);
}

Status ldx(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case IMMEDIATE: 
        line->byte_size = index16 ? 3 : 2;
        break;
      case ABSOLUTE:
        line->byte_size = 3;
        break;
      default: return invalid_operand(line);
      }
      return OK;
    }
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    line->byte_size = index16 ? 3 : 2;
    line->bytes[0] = LDX_IMM;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = LDX_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = LDX_ABS;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status longa(Line* line) {
  if(line->addr_mode != ABSOLUTE && line->expr1->type != SYMBOL)
    return invalid_operand(line);

  if(strcasecmp(line->expr1->e.sym, "on") == 0)
    acc16 = 1;
  else if(strcasecmp(line->expr1->e.sym, "off") == 0)
    acc16 = 0;
  else
    return invalid_operand(line);

  return OK;
}

Status longi(Line* line) {
  if(line->addr_mode != ABSOLUTE && line->expr1->type != SYMBOL)
    return invalid_operand(line);

  if(strcasecmp(line->expr1->e.sym, "on") == 0)
    index16 = 1;
  else if(strcasecmp(line->expr1->e.sym, "off") == 0)
    index16 = 0;
  else
    return invalid_operand(line);

  return OK;
}


Status lsr(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      line->byte_size = 3;
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ACCUMULATOR:
    line->byte_size = 1;
    line->bytes[0] = LSR_ACC;
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = LSR_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = LSR_ABS;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status org(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) 
    return error("ORG directive must be known on first pass");

  line->byte_size = 0;

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand <= 0xFFFFFF)
      pc = operand;
    else
      return operand_too_large(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}


Status pea(Line* line) {
  int operand;

  line->byte_size = 3;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    line->bytes[0] = PEA;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status pha(Line* line) { return implicit(line, PHA); }
Status pla(Line* line) { return implicit(line, PLA); }
Status pld(Line* line) { return implicit(line, PLD); }
Status ply(Line* line) { return implicit(line, PLY); }

Status rep(Line* line) {
  int operand;

  line->byte_size = 2;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    if(operand > 0xFF)
      return operand_too_large(operand);
    line->bytes[0] = REP;
    line->bytes[1] = LO(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status rtl(Line* line) { return implicit(line, RTL); }
Status rts(Line* line) { return implicit(line, RTS); }
Status sbc(Line* line) { return primary(line, SBC_BASE, acc16); }
Status sec(Line* line) { return implicit(line, SEC); }

Status sep(Line* line) {
  int operand;

  line->byte_size = 2;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    if(operand > 0xFF)
      return operand_too_large(operand);
    line->bytes[0] = SEP;
    line->bytes[1] = LO(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status sta(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case ABSOLUTE:
      case ABSOLUTE_INDEXED_X:
        line->byte_size = 4;
        return OK;
      default: return invalid_operand(line);
      }
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = STA_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = STA_ABS;
    }
    else if(operand <= 0xFFFFFF) {
      line->byte_size = 4;
      line->bytes[0] = STA_ABS_LONG;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    line->bytes[3] = HI(operand);
    break;
  case ABSOLUTE_INDEXED_X:
    if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = STA_ABS_INDEXED_X;
    }
    else if(operand <= 0xFFFFFF) {
      line->byte_size = 4;
      line->bytes[0] = STA_ABS_LONG_INDEXED_X;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    line->bytes[3] = HI(operand);
    break;
   default:
    return invalid_operand(line);
  }

  return OK;
}

Status stz(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      line->byte_size = 3;
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = STZ_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = STZ_ABS;
    }
    else
      return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status tas(Line* line) { return implicit(line, TAS); }
Status tax(Line* line) { return implicit(line, TAX); }
Status tsa(Line* line) { return implicit(line, TSA); }
Status txa(Line* line) { return implicit(line, TXA); }
Status xce(Line* line) { return implicit(line, XCE); }

