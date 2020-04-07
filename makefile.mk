IDIR=include
CC=gcc
CFLAGS=-I./$(IDIR) -g

ODIR=obj
SDIR=src
BDIR=bin

_DEPS = utils.h tokenizer.h lisp.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_SRC = utils.c tokenizer.c lisp.c main.c
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

_OBJ = $(_SRC:%.c=%.o)
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

lispint: $(OBJ)
	$(CC) -o $(BDIR)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 