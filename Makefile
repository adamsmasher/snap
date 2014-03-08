CFLAGS="-Wall"

snap: \
error.o \
eval.o \
expr.o \
handlers.o \
instructions.o \
labels.o \
lines.o \
parse.o \
snap.o \
table.o

error.o: \
error.c \
error.h \
lines.h \
snap.h

eval.o: \
eval.c \
eval.h \
expr.h \
labels.h \
snap.h

expr.o: \
expr.c \
expr.h

handlers.o: \
eval.h \
expr.h \
handlers.c \
handlers.h \
labels.h \
lines.h \
snap.h

instructions.o: \
error.h \
handlers.h \
instructions.c \
instructions.h \
table.h

labels.o: \
error.h \
labels.c \
labels.h \
snap.h \
table.h

lines.o: \
lines.c \
lines.h

parse.o: \
error.h \
instructions.h \
labels.h \
lines.h \
parse.c \
parse.h \
snap.h

snap.o: \
error.h \
instructions.h \
labels.h \
lines.h \
parse.h \
snap.c \
snap.h

table.o: \
table.c \
table.h
