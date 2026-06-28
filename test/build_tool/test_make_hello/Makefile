CC99 ?= cc99

.PHONY: all

all: hello

hello: hello.o
	$(CC99) -o hello hello.o

hello.o: hello.c
	$(CC99) -c -o hello.o hello.c
