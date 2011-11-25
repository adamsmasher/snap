#ifndef HANDLERS_H
#define HANDLERS_H

#include "error.h"
#include "lines.h"

Status adc(Line* line);
Status and(Line* line);
Status asl(Line* line);
Status beq(Line* line);
Status bit(Line* line);
Status bne(Line* line);
Status clc(Line* line);
Status dex(Line* line);
Status equ(Line* line);
Status inc(Line* line);
Status inx(Line* line);
Status jmp(Line* line);
Status jsr(Line* line);
Status lda(Line* line);
Status ldx(Line* line);
Status longa(Line* line);
Status longi(Line* line);
Status lsr(Line* line);
Status org(Line* line);
Status pea(Line* line);
Status pha(Line* line);
Status pla(Line* line);
Status pld(Line* line);
Status ply(Line* line);
Status rtl(Line* line);
Status rts(Line* line);
Status sbc(Line* line);
Status sec(Line* line);
Status sta(Line* line);
Status stz(Line* line);
Status tas(Line* line);
Status tax(Line* line);
Status tsa(Line* line);
Status txa(Line* line);
Status xce(Line* line);

#endif
