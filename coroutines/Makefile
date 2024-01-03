#
#
#

FLAGS = -ggdb -std=c++20 -O3

all: coro1 coro2

coro1: coro1.cpp Makefile 
	g++ coro1.cpp $(FLAGS) -lpthread -o coro1

coro2: coro2.cpp Makefile 
	g++ coro2.cpp $(FLAGS) -lpthread -o coro2

clean:
	-rm *.o coro1 coro2
