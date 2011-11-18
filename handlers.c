#include "handlers.h"

#include "expr.h"
#include "lines.h"
#include "snap.h"

#define PRIMARY_IMM 0x09
#define PRIMARY_DP 0x05
#define PRIMARY_ABS 0x0D
#define PRIMARY_ABS_LONG 0x0F

#define ADC_BASE 0x60

#define CLC 0x18

#define INC_ACC 0x1A
#define INC_DP 0xE6
#define INC_ABS 0xEE

#define LDA_BASE 0xA0

#define STZ_ABS 0x9C
#define STZ_DP 0x64

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

static Status primary(Line* line, int base, int sixteen_bit) {
  int operand;

  if(eval(line->expr1, &operand) != OK) {
    if(pass)
      return ERROR;
    else {
      /* set byte_size, assuming the worst */
      switch(line->addr_mode) {
      case IMMEDIATE:
        line->byte_size = 3;
        return OK;
      case ABSOLUTE:
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
  default:
    return invalid_operand(line);
  }

  return OK;
}

Status adc(Line* line) {
  return primary(line, ADC_BASE, acc16);
}

Status clc(Line* line) {
  switch(line->addr_mode) {
  case IMPLIED:
    line->byte_size = 1;
    line->bytes[0] = CLC;
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
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


Status lda(Line* line) {
  return primary(line, LDA_BASE, acc16);
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


Status xce(Line* line) {
  switch(line->addr_mode) {
  case IMPLIED:
    line->byte_size = 1;
    line->bytes[0] = XCE;
    break;
  default:
    return invalid_operand(line);
  }

  return OK;
}

