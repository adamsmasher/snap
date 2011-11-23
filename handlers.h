#ifndef HANDLERS_H
#define HANDLERS_H

#include "error.h"
#include "lines.h"

Status adc(Line* line);
Status and(Line* line);
Status bne(Line* line);
Status clc(Line* line);
Status inc(Line* line);
Status jsr(Line* line);
Status lda(Line* line);
Status org(Line* line);
Status sta(Line* line);
Status stz(Line* line);
Status tax(Line* line);
Status xce(Line* line);

#endif
