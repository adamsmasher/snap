#include "handlers.h"

#include "expr.h"
#include "eval.h"
#include "labels.h"
#include "lines.h"
#include "snap.h"

#include <string.h>
#include <strings.h>

#define PRIMARY_IMM 0x09
#define PRIMARY_ABS 0x0D
#define PRIMARY_ABS_LONG 0x0F
#define PRIMARY_DP 0x05
#define PRIMARY_DP_INDIRECT 0x12
#define PRIMARY_DP_INDIRECT_LONG 0x07
#define PRIMARY_ABS_INDEXED_X 0x1D
#define PRIMARY_ABS_LONG_INDEXED_X 0x1F
#define PRIMARY_ABS_INDEXED_Y 0x19
#define PRIMARY_DP_INDEXED_X 0x15
#define PRIMARY_DP_INDEXED_INDIRECT_X 0x01
#define PRIMARY_DP_INDIRECT_INDEXED_Y 0x11
#define PRIMARY_DP_INDIRECT_LONG_INDEXED_Y 0x17
#define PRIMARY_STACK_RELATIVE 0x03
#define PRIMARY_SR_INDIRECT_INDEXED 0x13

#define G2_ACC 0x2
#define G2_DP 0x1
#define G2_ABS 0x3
#define G2_DP_INDEXED 0x5
#define G2_ABS_INDEXED 0x7

#define INDEX_LOAD_IMM 0x00 
#define INDEX_LOAD_ABS 0x0C
#define INDEX_LOAD_DP 0x04
#define INDEX_LOAD_ABS_INDEXED 0x1C
#define INDEX_LOAD_DP_INDEXED 0x14

#define INDEX_CMP_IMM 0x00
#define INDEX_CMP_ABS 0x0C
#define INDEX_CMP_DP  0x04

#define TEST_ABS 0x0C
#define TEST_DP  0x04

#define ADC_BASE 0x60

#define AND_BASE 0x20

#define ASL_BASE 0x02

#define BCC 0x90

#define BCS 0xB0

#define BEQ 0xF0

#define BIT_IMM 0x89
#define BIT_ABS 0x2C
#define BIT_DP 0x24
#define BIT_ABS_INDEXED 0x3C
#define BIT_DP_INDEXED 0x34

#define BMI 0x30

#define BNE 0xD0

#define BPL 0x10

#define BRA 0x80

#define BRK 0x00

#define BRL 0x82

#define BVC 0x50

#define BVS 0x70

#define CLC 0x18

#define CLD 0xD8

#define CLI 0x58

#define CLV 0xB8

#define CMP_BASE 0xC0

#define COP 0x02

#define CPX_BASE 0xE0
#define CPY_BASE 0xC0

#define DEC_ACC 0x3A
#define DEC_BASE 0xC6

#define DEX 0xCA

#define DEY 0x88

#define EOR_BASE 0x40

#define INC_ACC 0x1A
#define INC_BASE 0xE6

#define INX 0xE8

#define INY 0xC8

#define JML_ABS      0x5C
#define JML_INDIRECT 0xDC

#define JMP_ABS                  0x4C
#define JMP_INDIRECT             0x6C
#define JMP_INDEXED_INDIRECT     0x7C

#define JSL 0x22

#define JSR_ABS                  0x20
#define JSR_ABS_INDEXED_INDIRECT 0xFC

#define LDA_BASE 0xA0

#define LDX_BASE 0xA2

#define LDY_BASE 0xA0

#define LSR_BASE 0x42

#define MVN 0x54

#define MVP 0x44

#define NOP 0xEA

#define ORA_BASE 0x00

#define PEA 0xF4

#define PEI 0xD4

#define PER 0x62

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

#define ROL_BASE 0x22

#define ROR_BASE 0x62

#define RTI 0x40

#define RTL 0x6B

#define RTS 0x60

#define SBC_BASE 0xE0

#define SEC 0x38

#define SED 0xF8

#define SEI 0x78

#define SEP 0xE2

#define STA_BASE 0x80

#define STP 0xDB

#define STX_BASE 0x86

#define STY_BASE 0x84

#define STZ_ABS 0x9C
#define STZ_DP 0x64
#define STZ_ABS_INDEXED 0x9E
#define STZ_DP_INDEXED 0x74

#define TAD 0x5B

#define TAS 0x1B

#define TAX 0xAA

#define TAY 0xA8

#define TDA 0x7B

#define TRB_BASE 0x10

#define TSA 0x3B

#define TSB_BASE 0x00

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

Status operand_out_of_range(int operand) {
  return error("operand %d out of range", operand);
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

Status relative_addr_out_of_bounds(Line* line) {
  if(line->expr1->type == SYMBOL)
    return error("relative address %s must be within same bank as instruction",
                 line->expr1->e.sym);  
  else
    return error("relative address must be within same bank as instruction");
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

static int is_direct_page(int* operand, Expr_class expr_class) {
  switch(expr_class) {
  case NUMERIC: return *operand <= 0xFF;
  case SYMBOLIC:
    if(*operand <= 0xFFFF && *operand >= d && *operand - d <= 0xFF) {
      *operand = *operand - d;
      return 1;
    } else {
      return 0;
    }
  }
}

static int is_near(int operand, Expr_class expr_class) {
  switch(expr_class) {
  case NUMERIC: return operand <= 0xFFFF;
  case SYMBOLIC: return operand >> 16 == dbr;
  }
}

static int is_far(int operand) {
  return operand <= 0xFFFFFF;
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
        if(base == STA_BASE) return invalid_operand(line);
        line->byte_size = sixteen_bit ? 3 : 2;
        return OK;
      case INDIRECT:
      case INDIRECT_LONG:
      case INDEXED_INDIRECT_X:
      case INDIRECT_INDEXED_Y:
      case INDIRECT_LONG_INDEXED_Y:
      case STACK_RELATIVE:
      case SR_INDIRECT_INDEXED:
        line->byte_size = 2;
        return OK;
      case ABSOLUTE_INDEXED_Y:
        line->byte_size = 3;
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
    if(base == STA_BASE) return invalid_operand(line);
    operand = immediate(operand, line->modifier, sixteen_bit);
    line->byte_size = sixteen_bit ? 3 : 2;
    line->bytes[0] = base + PRIMARY_IMM;
    if(sixteen_bit) line->bytes[2] = MID(operand);
    else if(operand > 0xFF)
      return operand_out_of_range(operand);
    break;
  case ABSOLUTE:
  case ABSOLUTE_INDEXED_X:
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = base + PRIMARY_DP; break;
      case ABSOLUTE_INDEXED_X: line->bytes[0] = base + PRIMARY_DP_INDEXED_X;
      default:;
      }
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = base + PRIMARY_ABS; break;
      case ABSOLUTE_INDEXED_X: line->bytes[0] = base + PRIMARY_ABS_INDEXED_X;
      default:;
      }
    }
    else if(is_far(operand)) {
      line->byte_size = 4;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = base + PRIMARY_ABS_LONG; break;
      case ABSOLUTE_INDEXED_X:
        line->bytes[0] = base + PRIMARY_ABS_LONG_INDEXED_X;
      default:;
      }
    }
    else
      return operand_out_of_range(operand);
    break;
  case ABSOLUTE_INDEXED_Y:
    line->byte_size = 3;
    if(!is_near(operand, expr_class(line->expr1)))
      return operand_out_of_range(operand);
    line->bytes[0] = base + PRIMARY_ABS_INDEXED_Y;
    break;
  case INDIRECT:
  case INDIRECT_LONG:
  case INDEXED_INDIRECT_X:
  case INDIRECT_INDEXED_Y:
  case INDIRECT_LONG_INDEXED_Y:
  case SR_INDIRECT_INDEXED:
  case STACK_RELATIVE:
    line->byte_size = 2;
    if(operand > 0xFF)
      return operand_out_of_range(operand);
    switch(line->addr_mode) {
    case INDIRECT: line->bytes[0] = base + PRIMARY_DP_INDIRECT; break;
    case INDIRECT_LONG: line->bytes[0] = base + PRIMARY_DP_INDIRECT_LONG; break;
    case INDIRECT_INDEXED_Y:
      line->bytes[0] = base + PRIMARY_DP_INDIRECT_INDEXED_Y;
      break;
    case INDEXED_INDIRECT_X:
      line->bytes[0] = base + PRIMARY_DP_INDEXED_INDIRECT_X;
      break;
    case INDIRECT_LONG_INDEXED_Y:
      line->bytes[0] = base + PRIMARY_DP_INDIRECT_LONG_INDEXED_Y;
      break;
    case STACK_RELATIVE: line->bytes[0] = base + PRIMARY_STACK_RELATIVE; break;
    case SR_INDIRECT_INDEXED:
      line->bytes[0] = base + PRIMARY_SR_INDIRECT_INDEXED;
    default:;
    }
    break;
  default:
    return invalid_operand(line);
  }

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);
  line->bytes[3] = HI(operand);
  return OK;
}

Status group2(Line* line, int base) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case ACCUMULATOR:
        line->byte_size = 1;
        return OK;
      case ABSOLUTE:
      case ABSOLUTE_INDEXED_X:
        line->byte_size = 3;
        return OK;
      default: return invalid_operand(line);
      }
    }
  }

  switch(line->addr_mode) {
  case ACCUMULATOR:
    line->byte_size = 1;
    switch(base) {
    case INC_BASE: line->bytes[0] = INC_ACC; break;
    case DEC_BASE: line->bytes[0] = DEC_ACC; break;
    case ASL_BASE:
    case LSR_BASE:
    case ROL_BASE:
    case ROR_BASE:
      line->bytes[0] = base | (G2_ACC << 2);
      break;
   default: return invalid_operand(line);
   }
   break;
  case ABSOLUTE:
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      line->bytes[0] = base | (G2_DP << 2);
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      line->bytes[0] = base | (G2_ABS << 2);
    }
    else
      return operand_out_of_range(operand);
    break;
  case ABSOLUTE_INDEXED_X:
  case ABSOLUTE_INDEXED_Y:
    if(line->addr_mode == ABSOLUTE_INDEXED_Y && base != STX_BASE)
      return invalid_operand(line);
    if(line->addr_mode == ABSOLUTE_INDEXED_X && base == STX_BASE)
      return invalid_operand(line);

    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      line->bytes[0] = base | (G2_DP_INDEXED << 2);
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      line->bytes[0] = base | (G2_ABS_INDEXED << 2);
    }
    else
      return operand_out_of_range(operand);
    break;
  default:
    return invalid_operand(line);
  }

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);
  return OK;
}

Status indexld(Line* line, int base) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case IMMEDIATE: 
        line->byte_size = index16 ? 3 : 2;
        return OK;
      case ABSOLUTE:
        line->byte_size = 3;
        return OK;
      case ABSOLUTE_INDEXED_X:
      case ABSOLUTE_INDEXED_Y:
        if((line->addr_mode == ABSOLUTE_INDEXED_X && base == LDX_BASE) ||
           (line->addr_mode == ABSOLUTE_INDEXED_Y && base == LDY_BASE))
          return invalid_operand(line);
        line->byte_size = 3;
        return OK;
      default: return invalid_operand(line);
      }
    }
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    operand = immediate(operand, line->modifier, index16);
    line->byte_size = index16 ? 3 : 2;
    line->bytes[0] = base + INDEX_LOAD_IMM;
    break;
  case ABSOLUTE:
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      line->bytes[0] = base + INDEX_LOAD_DP;
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      line->bytes[0] = base + INDEX_LOAD_ABS;
    }
    else
      return operand_out_of_range(operand);
    break;
  case ABSOLUTE_INDEXED_X:
  case ABSOLUTE_INDEXED_Y:
    if((line->addr_mode == ABSOLUTE_INDEXED_X && base == LDX_BASE) ||
       (line->addr_mode == ABSOLUTE_INDEXED_Y && base == LDY_BASE))
      return invalid_operand(line);
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      line->bytes[0] = base + INDEX_LOAD_DP_INDEXED;
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      line->bytes[0] = base + INDEX_LOAD_ABS_INDEXED;
    }
    else
      return operand_out_of_range(operand);
    break;
  default:
    return invalid_operand(line);
  }

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);
  return OK;
}

Status indexcmp(Line* line, int base) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case IMMEDIATE:
        line->byte_size = index16 ? 3 : 2;
        return OK;
      case ABSOLUTE:
        line->byte_size = 3;
        return OK;
      default: return invalid_operand(line);
      }
    }
  }

  switch(line->addr_mode) {
  case IMMEDIATE:
    operand = immediate(operand, line->modifier, 0);
    if(operand > 0xFFFF || (!index16 && operand > 0xFF))
      return operand_out_of_range(operand);
    line->byte_size = index16 ? 3 : 2;
    line->bytes[0] = base + INDEX_CMP_IMM;
    break;
  case ABSOLUTE:
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      line->bytes[0] = base + INDEX_CMP_DP;
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      line->bytes[0] = base + INDEX_CMP_ABS;
    }
    else
      return operand_out_of_range(operand);
    break;
  default: return invalid_operand(line);
  }

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);
  return OK;
}

Status testbits(Line* line, int base) {
  int operand;

   if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      line->byte_size = 3;
      return OK;
    }
  }

  if(line->addr_mode != ABSOLUTE)
    return invalid_operand(line);
  if(is_direct_page(&operand, expr_class(line->expr1))) {
    line->bytes[0] = base + TEST_DP;
    line->byte_size = 2;
  }
  else if(is_near(operand, expr_class(line->expr1))) {
    line->bytes[0] = base + TEST_ABS;
    line->byte_size = 3;
  }
  else
    return operand_out_of_range(operand);

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);

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

Status move(Line* line, int op) {
  int op1;
  int op2;

  line->byte_size = 3;
  if(line->addr_mode != LIST)
    return invalid_operand(line);

  if(line->expr1->e.num != 2)
    return invalid_operand(line);

  if(eval(line->expr2, &op1) != OK ||
     eval(line->expr2->next, &op2) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  if(op1 > 0xFFFFFF)
    return operand_out_of_range(op1);
  if(op2 > 0xFFFFFF)
    return operand_out_of_range(op2);

  line->bytes[0] = op;
  line->bytes[1] = HI(op2);
  line->bytes[2] = HI(op1);

  return OK;
}

Status constant(Line* line, int op) {
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
      return operand_out_of_range(operand);
    line->bytes[0] = op;
    line->bytes[1] = LO(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status adc(Line* line) { return primary(line, ADC_BASE, acc16); }
Status and(Line* line) { return primary(line, AND_BASE, acc16); }

Status ascii(Line* line) {
  if(line->addr_mode != STRING)
    return invalid_operand(line);

  line->byte_size = strlen(line->expr1->e.str);

  return OK;
}

Status asl(Line* line) { return group2(line, ASL_BASE); }
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
      case ABSOLUTE_INDEXED_X:
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
    break;
  case ABSOLUTE:
  case ABSOLUTE_INDEXED_X:
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = BIT_DP; break;
      case ABSOLUTE_INDEXED_X: line->bytes[0] = BIT_DP_INDEXED;
      default:;
      }
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = BIT_ABS; break;
      case ABSOLUTE_INDEXED_X: line->bytes[0] = BIT_ABS_INDEXED;
      default:;
      }
    }
    else
      return operand_out_of_range(operand);
    break;
  default:
    return invalid_operand(line);
  }

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);
  return OK;
}

Status bmi(Line* line) { return branch(line, BMI); }
Status bne(Line* line) { return branch(line, BNE); }
Status bpl(Line* line) { return branch(line, BPL); }
Status bra(Line* line) { return branch(line, BRA); }
Status brk(Line* line) { return constant(line, BRK); }

Status brl(Line* line) {
  int operand;
  char dest;

  line->byte_size = 3;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    if(operand - pc - 3 >= 32768 || operand - pc - 8 < -32768)
      return branch_out_of_bounds(line);
    dest = (char)(operand - pc - 3);
    line->bytes[0] = BRL;;
    line->bytes[1] = LO(dest);
    line->bytes[2] = MID(dest);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status bvc(Line* line) { return branch(line, BVC); }
Status bvs(Line* line) { return branch(line, BVS); }
Status clc(Line* line) { return implicit(line, CLC); }
Status cld(Line* line) { return implicit(line, CLD); }
Status cli(Line* line) { return implicit(line, CLI); }
Status clv(Line* line) { return implicit(line, CLV); }
Status cmp(Line* line) { return primary(line, CMP_BASE, acc16); }
Status cop(Line* line) { return constant(line, COP); }
Status cpx(Line* line) { return indexcmp(line, CPX_BASE); }
Status cpy(Line* line) { return indexcmp(line, CPY_BASE); }

Status db(Line* line) {
  int operand;

  if(line->addr_mode == ABSOLUTE) {
    line->byte_size = 1;
    if(eval(line->expr1, &operand) != OK) {
      if(pass)
        return ERROR;
      else
        return OK;
    }
    if(operand > 0xFF)
      return operand_out_of_range(operand);
    
    line->bytes[0] = LO(operand);
  
    return OK;
  }
  else if(line->addr_mode == LIST) {
    Expr* e = line->expr2;
    line->byte_size = line->expr1->e.num;
    /* attempt to eval all of the words */
    while(e) {
      if(eval(e, &operand) != OK) {
        if(pass)
          return ERROR;
        else
          return OK;
      }
      if(operand > 0xFF)
        return operand_out_of_range(operand);
      e->type = NUMBER;
      e->e.num = operand;
      e = e->next;
    }
    /* leave it to the final assembler to write them */
    return OK;
  }
  else
    return invalid_operand(line);
}

Status dw(Line* line) {
  int operand;

  if(line->addr_mode == ABSOLUTE) {
    line->byte_size = 2;
    if(eval(line->expr1, &operand) != OK) {
      if(pass)
        return ERROR;
      else
        return OK;
    }
    if(operand > 0xFFFF)
      return operand_out_of_range(operand);
    
    line->bytes[0] = LO(operand);
    line->bytes[1] = MID(operand);
  
    return OK;
  }
  else if(line->addr_mode == LIST) {
    Expr* e = line->expr2;
    line->byte_size = line->expr1->e.num * 2;
    /* attempt to eval all of the words */
    while(e) {
      if(eval(e, &operand) != OK) {
        if(pass)
          return ERROR;
        else
          return OK;
      }
      if(operand > 0xFFFF)
        return operand_out_of_range(operand);
      e->type = NUMBER;
      e->e.num = operand;
      e = e->next;
    }
    /* leave it to the final assembler to write them */
    return OK;
  }
  else
    return invalid_operand(line);
}

Status dec(Line* line) { return group2(line, DEC_BASE); }
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

Status inc(Line* line) { return group2(line, INC_BASE); }

Status incbin(Line* line) {
  FILE* fp;

  if(line->addr_mode != STRING)
    return invalid_operand(line);

  fp = fopen(line->expr1->e.str, "rb");
  if(!fp)
    return error("cannot open included file %s", line->expr1->e.str);

  fseek(fp, 0L, SEEK_END);
  line->byte_size = ftell(fp);

  fclose(fp);

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
  case INDIRECT:
  case INDEXED_INDIRECT_X:
    if(HI(operand) != HI(pc))
      return jump_out_of_bounds(line);
    switch(line->addr_mode) {
    case ABSOLUTE: line->bytes[0] = JMP_ABS; break;
    case INDIRECT: line->bytes[0] = JMP_INDIRECT; break;
    case INDEXED_INDIRECT_X: line->bytes[0] = JMP_INDEXED_INDIRECT;
    default:;
    }
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    break;
  default: return invalid_operand(line);
  }

  return OK;
}

Status jml(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      switch(line->addr_mode) {
      case ABSOLUTE: line->byte_size = 4;
      case INDIRECT_LONG: line->byte_size = 3;
      default: invalid_operand(line);
      }
      return OK;
    }
  }

  switch(line->addr_mode) {
  case ABSOLUTE:
    line->byte_size = 4;
    if(operand > 0xFFFFFF)
      return operand_out_of_range(operand);
    line->bytes[0] = JML_ABS;
    line->bytes[1] = LO(operand);
    line->bytes[2] = MID(operand);
    line->bytes[3] = HI(operand);
    break;
  case INDIRECT_LONG:
    line->byte_size = 3;
    if(!is_near(operand, expr_class(line->expr1)))
      return operand_out_of_range(operand);
    line->bytes[0] = JML_INDIRECT;
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
      return operand_out_of_range(operand);
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
    break;
  case INDEXED_INDIRECT_X:
    if(!is_near(operand, expr_class(line->expr1)))
      return operand_out_of_range(operand);
    line->bytes[0] = JSR_ABS_INDEXED_INDIRECT;
    break;
  default: return invalid_operand(line);
  }

  line->bytes[1] = LO(operand);
  line->bytes[2] = MID(operand);
  return OK;
}


Status lda(Line* line) {
  return primary(line, LDA_BASE, acc16);
}

Status ldx(Line* line) { return indexld(line, LDX_BASE); }
Status ldy(Line* line) { return indexld(line, LDY_BASE); }

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

Status lsr(Line* line) { return group2(line, LSR_BASE); }
Status mvn(Line* line) { return move(line, MVN); }
Status mvp(Line* line) { return move(line, MVP); }
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
      return operand_out_of_range(operand);
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

Status pei(Line* line) {
  int operand;

  line->byte_size = 2;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  if(line->addr_mode != INDIRECT)
    return invalid_operand(line);
  if(!is_direct_page(&operand, expr_class(line->expr1)))
    return operand_out_of_range(operand);

  line->bytes[0] = PEI;
  line->bytes[1] = LO(operand);
  return OK;
}

Status per(Line* line) {
  int operand;
  int displace;

  line->byte_size = 3;
  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }

  if(HI(pc) != HI(operand))
    return relative_addr_out_of_bounds(line);

  displace = operand - pc;

  line->bytes[0] = PER;
  line->bytes[1] = MID(displace);
  line->bytes[2] = LO(displace);

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
Status rep(Line* line) { return constant(line, REP); }
Status rol(Line* line) { return group2(line, ROL_BASE); }
Status ror(Line* line) { return group2(line, ROR_BASE); }
Status rti(Line* line) { return implicit(line, RTI); }
Status rtl(Line* line) { return implicit(line, RTL); }
Status rts(Line* line) { return implicit(line, RTS); }
Status sbc(Line* line) { return primary(line, SBC_BASE, acc16); }
Status sec(Line* line) { return implicit(line, SEC); }
Status sed(Line* line) { return implicit(line, SED); }
Status sei(Line* line) { return implicit(line, SEI); }
Status sep(Line* line) { return constant(line, SEP); }

Status setd(Line* line) {
  int operand;

  if(line->addr_mode != IMMEDIATE)
    return invalid_operand(line);

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else
      return OK;
  }
  switch(line->modifier) {
  case IMMEDIATE_HI: operand = (operand & 0xFFFF00) >> 8; break;
  case IMMEDIATE_MID: operand = operand & 0xFFFF; break;
  case IMMEDIATE_LO: operand = operand & 0xFFFF; break;
  case NONE: break;
  }
  if(operand > 0xFFFF)
    return operand_out_of_range(operand);
  d = operand;
  return OK;
}

Status sta(Line* line) { return primary(line, STA_BASE, 0); }
Status stp(Line* line) { return implicit(line, STP); }
Status stx(Line* line) { return group2(line, STX_BASE); }
Status sty(Line* line) { return group2(line, STY_BASE); }

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
  case ABSOLUTE_INDEXED_X:
    if(is_direct_page(&operand, expr_class(line->expr1))) {
      line->byte_size = 2;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = STZ_DP; break;
      case ABSOLUTE_INDEXED_X: line->bytes[0] = STZ_DP_INDEXED;
      default:;
      }
    }
    else if(is_near(operand, expr_class(line->expr1))) {
      line->byte_size = 3;
      switch(line->addr_mode) {
      case ABSOLUTE: line->bytes[0] = STZ_ABS; break;
      case ABSOLUTE_INDEXED_X: line->bytes[0] = STZ_ABS_INDEXED;
      default:;
      }
    }
    else
      return operand_out_of_range(operand);
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
Status trb(Line* line) { return testbits(line, TRB_BASE); }
Status tsa(Line* line) { return implicit(line, TSA); }
Status tsb(Line* line) { return testbits(line, TSB_BASE); }
Status tsx(Line* line) { return implicit(line, TSX); }
Status txa(Line* line) { return implicit(line, TXA); }
Status txs(Line* line) { return implicit(line, TXS); }
Status txy(Line* line) { return implicit(line, TXY); }
Status tya(Line* line) { return implicit(line, TYA); }
Status tyx(Line* line) { return implicit(line, TYX); }
Status wai(Line* line) { return implicit(line, WAI); }
Status xba(Line* line) { return implicit(line, XBA); }
Status xce(Line* line) { return implicit(line, XCE); }

