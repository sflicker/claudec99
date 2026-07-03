CC99 ?= cc99

.PHONY: all

all: prog

prog: prog.o
	$(CC99) -o prog prog.o

prog.o: prog.c
	$(CC99) -c -o prog.o prog.c
