//--- link.cpp -std=c++20
#include "common.hpp"
#include <cassert>
#include <iostream>

int main()
{
    const int* x1 = func1();
    const int* x2 = func2();
    assert(  x1 !=  x2 ); // !! different addresses in memory
    assert( *x1 == *x2 );
    assert( *x1 == 42 );
    return 0;
}

