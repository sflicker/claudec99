CC99  ?= cc99
CFLAGS = -std=c99 -O2 -Wall

.PHONY: all

all: main

main: main.o
	$(CC99) -o main main.o

main.o: main.c
	$(CC99) $(CFLAGS) -c -o main.o main.c
