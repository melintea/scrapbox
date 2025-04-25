// -std=c++20

#include <iostream>

static_assert(sizeof(int) == 4);
constexpr int ub(int i) { return i*i; }

// compile error
//constinit const int cci(ub(1<<16)); 

// compile error
consteval int cub(int i) { return i*i; }
//const int cci(cub(1<<16)); 

int main()
{
    // clang: runtime: 0
    // gcc:   idem
    const int ci(ub(1<<16)); std::cout << "ci=" << ci << '\n';

    // clang: compile error
    // gcc:   idem
    //constexpr const int cci(ub(1<<16)); std::cout << "cci=" << cci << '\n';
    
    return 0;
}
