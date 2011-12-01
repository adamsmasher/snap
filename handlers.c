#include "handlers.h"

#include "expr.h"
#include "eval.h"
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
#define PRIMARY_DP_INDEXED_X 0x15
#define PRIMARY_STACK_RELATIVE 0x03

#define ADC_BASE 0x60

#define AND_BASE 0x20

#define ASL_ACC 0x0A
#define ASL_DP 0x06
#define ASL_ABS 0x0E

#define BCC 0x90

#define BCS 0xB0

#define BEQ 0xF0

#define BIT_IMM 0x89
#define BIT_DP 0x24
#define BIT_ABS 0x2C

#define BMI 0x30

#define BNE 0xD0

#define BPL 0x10

#define BRA 0x80

#define BVC 0x50

#define BVS 0x70

#define CLC 0x18

#define CLD 0xD8

#define CLI 0x58

#define CLV 0xB8

#define CMP_BASE 0xC0

#define DEC_ACC 0x3A
#define DEC_DP 0xC6
#define DEC_ABS 0xCE

#define DEX 0xCA

#define DEY 0x88

#define EOR_BASE 0x40

#define INC_ACC 0x1A
#define INC_DP 0xE6
#define INC_ABS 0xEE

#define INX 0xE8

#define INY 0xC8

#define JMP_ABS                  0x4C
#define JMP_ABS_INDEXED_INDIRECT 0x7C

#define JSL 0x22

#define JSR_ABS                  0x20
#define JSR_ABS_INDEXED_INDIRECT 0xFC

#define LDA_BASE 0xA0

#define LDX_IMM 0xA2
#define LDX_DP 0xA6
#define LDX_ABS 0xAE

#define LSR_ACC 0x4A
#define LSR_ABS 0x4E
#define LSR_DP 0x46

#define NOP 0xEA

#define ORA_BASE 0x00

#define PEA 0xF4

#define PHA 0x48

#define PHB 0x8B

#define PHD 0x0B

#define PHK 0x4B

#define PHP 0x08

#define PHX 0xDA

#define PHY 0x5A

#define PLA 0x68

#define PLB  0xAB

#define PLD 0x2B

#define PLP 0x28

#define PLX 0xFA

#define PLY 0x7A

#define REP 0xC2

#define RTI 0x40

#define RTL 0x6B

#define RTS 0x60

#define SBC_BASE 0xE0

#define SEC 0x38

#define SED 0xF8

#define SEI 0x78

#define SEP 0xE2

#define STA_DP 0x85
#define STA_ABS 0x8D
#define STA_ABS_LONG 0x8F
#define STA_ABS_INDEXED_X 0x9D
#define STA_ABS_LONG_INDEXED_X 0x9F
#define STA_INDEXED_INDIRECT_X 0x81

#define STP 0xDB

#define STY_ABS 0x8C
#define STY_DP 0x84
#define STY_DP_INDEXED_X 0x94

#define STZ_ABS 0x9C
#define STZ_DP 0x64

#define TAD 0x5B

#define TAS 0x1B

#define TAX 0xAA

#define TAY 0xA8

#define TDA 0x7B

#define TSA 0x3B

#define TSX 0xBA

#define TXA 0x8A

#define TXS 0x9A

#define TXY 0x9B

#define TYA 0x98

#define TYX 0xBB

#define WAI 0xCB

#define XBA 0xEB

#define XCE 0xFB

#define LO(x) ((char)(x))
#define MID(x) ((char)((x) >> 8))
#define HI(x) ((char)((x) >> 16))

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

static int immediate(int operand, Addressing_modifier mod, int sixteen) {
  if(sixteen) {
    switch(mod) {
    case IMMEDIATE_HI: return (operand && 0xFFFF00) >> 8;
    case IMMEDIATE_MID: return operand && 0xFFFF;
    case IMMEDIATE_LO: return operand && 0xFFFF;
    default: return operand;
    }
  }
  else {
    switch(mod) {
    case IMMEDIATE_HI: return (operand && 0xFF0000) >> 16;
    case IMMEDIATE_MID: return (operand && 0x00FF00) >> 8;
    case IMMEDIATE_LO: return operand && 0xFF;
    default: return operand;
    }
  }
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
      case STACK_RELATIVE:
        line->byte_size = 2;
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
    operand = immediate(operand, line->modifier, sixteen_bit);
    line->byte_size = sixteen_bit ? 3 : 2;
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
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = base + PRIMARY_DP_INDEXED_X;
    }
    else if(operand <= 0xFFFF) {
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
  case STACK_RELATIVE:
    line->byte_size = 2;
    if(operand > 0xFF)
      return operand_too_large(operand);
    line->bytes[0] = base + PRIMARY_STACK_RELATIVE;
    line->bytes[1] = LO(operand);
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

Status branch(Line* line, int op) {
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
    if(operand - pc - 2 >= 128 || operand - pc - 2 < -128)
      return branch_out_of_bounds(line);
    dest = (char)(operand - pc - 2);
    line->bytes[0] = op;
    line->bytes[1] = dest;
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status adc(Line* line) {
  return primary(line, ADC_BASE, acc16);
}

Status and(Line* line) {
  return primary(line, AND_BASE, acc16);
}

Status ascii(Line* line) {
  if(line->addr_mode != STRING)
    return invalid_operand(line);

  line->byte_size = strlen(line->expr1->e.str);

  return OK;
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

Status bcc(Line* line) { return branch(line, BCC); }
Status bcs(Line* line) { return branch(line, BCS); }
Status beq(Line* line) { return branch(line, BEQ); }

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
    operand = immediate(operand, line->modifier, acc16);
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

Status bmi(Line* line) { return branch(line, BMI); }
Status bne(Line* line) { return branch(line, BNE); }
Status bpl(Line* line) { return branch(line, BPL); }
Status bra(Line* line) { return branch(line, BRA); }
Status bvc(Line* line) { return branch(line, BVC); }
Status bvs(Line* line) { return branch(line, BVS); }
Status clc(Line* line) { return implicit(line, CLC); }
Status cld(Line* line) { return implicit(line, CLD); }
Status cli(Line* line) { return implicit(line, CLI); }
Status clv(Line* line) { return implicit(line, CLV); }
Status cmp(Line* line) { return primary(line, CMP_BASE, acc16); }

Status db(Line* line) {
  int operand;

  line->byte_size = 1;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  if(line->addr_mode != ABSOLUTE)
    return invalid_operand(line);

  if(operand > 0xFF)
    return operand_too_large(operand);

  line->bytes[0] = LO(operand);

  return OK;
}

Status dw(Line* line) {
  int operand;

  line->byte_size = 2;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  if(line->addr_mode != ABSOLUTE)
    return invalid_operand(line);

  if(operand > 0xFFFF)
    return operand_too_large(operand);

  line->bytes[0] = LO(operand);
  line->bytes[1] = MID(operand);

  return OK;
}

Status dec(Line* line) {
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
    line->bytes[0] = DEC_ACC;
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = DEC_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = DEC_ABS;
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

Status dex(Line* line) { return implicit(line, DEX); }
Status dey(Line* line) { return implicit(line, DEY); }
Status eor(Line* line) { return primary(line, EOR_BASE, acc16); }

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
Status iny(Line* line) { return implicit(line, INY); }

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

Status jsl(Line* line) {
  int operand;

  line->byte_size = 4;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand > 0xFFFFFF)
      return operand_too_large(operand);
    line->bytes[0] = JSL;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    line->bytes[3] = HI(operand);
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
    operand = immediate(operand, line->modifier, index16);
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

Status nop(Line* line) { return implicit(line, NOP); }
Status ora(Line* line) { return primary(line, ORA_BASE, acc16); }

Status org(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) 
    return error("ORG operand must be known on first pass");

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

Status pad(Line* line) {
  int operand1;

  if(eval(line->expr1, &operand1) != OK)
    return error("PAD operand must be known on first pass");

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand1 - pc < 0)
      return error("PAD length must be positive");
    line->byte_size = operand1 - pc;
    break;
  default: return invalid_operand(line);
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
Status phb(Line* line) { return implicit(line, PHB); }
Status phd(Line* line) { return implicit(line, PHD); }
Status phk(Line* line) { return implicit(line, PHK); }
Status php(Line* line) { return implicit(line, PHP); }
Status phx(Line* line) { return implicit(line, PHX); }
Status phy(Line* line) { return implicit(line, PHY); }
Status pla(Line* line) { return implicit(line, PLA); }
Status plb(Line* line) { return implicit(line, PLB); }
Status pld(Line* line) { return implicit(line, PLD); }
Status plp(Line* line) { return implicit(line, PLP); }
Status plx(Line* line) { return implicit(line, PLX); }
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
    operand = immediate(operand, line->modifier, 0);
    if(operand > 0xFF)
      return operand_too_large(operand);
    line->bytes[0] = REP;
    line->bytes[1] = LO(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status rti(Line* line) { return implicit(line, RTI); }
Status rtl(Line* line) { return implicit(line, RTL); }
Status rts(Line* line) { return implicit(line, RTS); }
Status sbc(Line* line) { return primary(line, SBC_BASE, acc16); }
Status sec(Line* line) { return implicit(line, SEC); }
Status sed(Line* line) { return implicit(line, SED); }
Status sei(Line* line) { return implicit(line, SEI); }

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
    operand = immediate(operand, line->modifier, 0);
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
      case INDEXED_INDIRECT_X:
        line->byte_size = 2;
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
  case INDEXED_INDIRECT_X:
    if(operand > 0xFF)
      return operand_too_large(operand);
    line->byte_size = 2;
    line->bytes[0] = STA_INDEXED_INDIRECT_X;
    line->bytes[1] = LO(operand); 
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status stp(Line* line) { return implicit(line, STP); }

Status sty(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      switch(line->addr_mode) {
      case ABSOLUTE:
        line->byte_size = 3;
        return OK;
      case ABSOLUTE_INDEXED_X:
        line->byte_size = 2;
        return OK;
      default: return invalid_operand(line);
      }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = STY_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = STY_ABS;
    }
    else return operand_too_large(operand);
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  case ABSOLUTE_INDEXED_X:
    if(operand > 0xFF) return operand_too_large(operand);
    line->byte_size = 2;
    line->bytes[0] = STY_DP_INDEXED_X;
    line->bytes[1] = LO(operand);
    break;
  default: return invalid_operand(line);
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

Status tad(Line* line) { return implicit(line, TAD); }
Status tas(Line* line) { return implicit(line, TAS); }
Status tax(Line* line) { return implicit(line, TAX); }
Status tay(Line* line) { return implicit(line, TAY); }
Status tda(Line* line) { return implicit(line, TDA); }
Status tsa(Line* line) { return implicit(line, TSA); }
Status tsx(Line* line) { return implicit(line, TSX); }
Status txa(Line* line) { return implicit(line, TXA); }
Status txs(Line* line) { return implicit(line, TXS); }
Status txy(Line* line) { return implicit(line, TXY); }
Status tya(Line* line) { return implicit(line, TYA); }
Status tyx(Line* line) { return implicit(line, TYX); }
Status wai(Line* line) { return implicit(line, WAI); }
Status xba(Line* line) { return implicit(line, XBA); }
Status xce(Line* line) { return implicit(line, XCE); }

