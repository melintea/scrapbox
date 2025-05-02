#!/bin/bash

g++ -ggdb -fpermissive -fPIC -rdynamic -shared -o libdll1.so dll1.cpp 
g++ -ggdb -fpermissive -fPIC -rdynamic -shared -o libdll2.so dll2.cpp 
g++ -ggdb -fpermissive -fPIC -rdynamic -shared -o libdll3.so dll3.cpp 

# linking order decides if Global2 (or 3) is present in the dll1::_global1._tags
# -ldll1 -ldll2 would kill erase "Global2" from _tags as _global1 will be inited twice:
#   - once when dll2 is loaded -> it loads and initializes dll1 as dependency and inserts Global2
#   - again when dll1 is loaded (again!)

echo "*** Good order: -ldll3 -ldll2 -ldll1"
g++ -ggdb -fpermissive -fPIC -rdynamic -o main main.cpp -L. -ldll3 -ldll2 -ldll1 
LD_LIBRARY_PATH=. ./main || exit 1

echo "*** Wipes two (dll2/3): -ldll1 -ldll3 -ldll2"
g++ -ggdb -fpermissive -fPIC -rdynamic -o main main.cpp -L. -ldll1 -ldll3 -ldll2
LD_LIBRARY_PATH=. ./main || exit 1

echo "*** Wipes one (dll2): -ldll3 -ldll1 -ldll2"
g++ -ggdb -fpermissive -fPIC -rdynamic -o main main.cpp -L. -ldll3 -ldll1 -ldll2
LD_LIBRARY_PATH=. ./main || exit 1


rm *.so main
