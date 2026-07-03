CC99 ?= cc99
CFLAGS = --testinclude

.PHONY: all

all: prog

prog: prog.o
	$(CC99) -o prog prog.o

prog.o: prog.c
	$(CC99) $(CFLAGS) -c -o prog.o prog.c
