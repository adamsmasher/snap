#ifndef HANDLERS_H
#define HANDLERS_H

#include "error.h"
#include "lines.h"

Status adc(Line* line);
Status clc(Line* line);
Status lda(Line* line);
Status org(Line* line);
Status stz(Line* line);
Status xce(Line* line);

#endif
