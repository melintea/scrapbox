#
#
#
#

ALL = anews amail

CFLAGS = -ggdb
LIBS   = -lpthread

all: $(ALL)

anews: sendnews.o
	g++ $(CFLAGS) -o anews $+ $(LIBS)

amail: sendnews.o
	g++ $(CFLAGS) -o amail $+ $(LIBS)


.cpp.o:
	g++ $(CFLAGS) -c $+ 

.c.o:
	gcc $(CFLAGS) -c $+ 


clean:
	rm *.o $(ALL)

