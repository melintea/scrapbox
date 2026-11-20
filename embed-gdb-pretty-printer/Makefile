#
#
#

FLAGS  =  -g 
TARGET = exetest

all:  test

test: $(TARGET)
	/usr/bin/gdb -batch \
	             -ex "info pretty-printer" \
	             -ex "quit" \
		     --args $(TARGET) | grep "LocationPrinter"
	/usr/bin/gdb -batch \
	             -ex "b test" \
	             -ex "run" \
		     -ex "print p" -ex "continue" \
		     -ex "quit" \
		     --args $(TARGET) | grep "Location="
	@echo "== Pass =="

$(TARGET): *.cpp gdb-ppe.h Makefile 
	g++ *.cpp $(FLAGS) -o $@
	
gdb-ppe.h: gdb-ppe.py  embed-gdb-extension.py
	./embed-gdb-extension.py $< > $@

clean:
	-rm *.o $(TARGET) gdb-ppe.h
