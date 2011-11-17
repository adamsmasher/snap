#include "handlers.h"

#include "expr.h"
#include "lines.h"
#include "snap.h"

#define ADC_IMM 0x69
#define ADC_DP 0x65
#define ADC_ABS 0x6D
#define ADC_ABS_LONG 0x6F

#define CLC 0x18

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

Status adc(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK)
    return ERROR;

  switch(line->addr_mode) {
  case IMMEDIATE:
    line->byte_size = acc16 ? 3 : 2;
    line->bytes[0] = ADC_IMM;
    line->bytes[1] = LO(operand);
    if(acc16) line->bytes[2] = MID(operand);
    else if(operand > 0xFF)
      return operand_too_large(operand);
    break;
  case ABSOLUTE:
    if(operand <= 0xFF) {
      line->byte_size = 2;
      line->bytes[0] = ADC_DP;
    }
    else if(operand <= 0xFFFF) {
      line->byte_size = 3;
      line->bytes[0] = ADC_ABS;
    }
    else if(operand <= 0xFFFFFF) {
      line->byte_size = 4;
      line->bytes[0] = ADC_ABS_LONG;
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

Status org(Line* line) {
  int operand;

  if(eval(line->expr1, &operand) != OK)
    return ERROR;

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

