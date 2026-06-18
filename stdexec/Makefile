#
#
#

FLAGS = -ggdb -std=c++20 -I$(HOME)/work/stdexec/include

all: executors1	 

executors1: executors1.cpp Makefile 
	clang++ executors1.cpp $(FLAGS) -lpthread -o executors1

clean:
	-rm *.o executors1 
