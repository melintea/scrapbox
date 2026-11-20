#
#
#

CFLAGS = -ggdb 
CC     = gcc # or clang

all: cguard

cguard: cguard.c Makefile 
	$(CC) cguard.c $(FLAGS) -o cguard

test: cguard
	./cguard

clean:
	-rm *.o cguard
