#include <cassert>
#include <iostream>

#include "dll1.hpp"
#include "dll2.hpp"
#include "dll3.hpp"


const dll2::Global2& g2ref = dll2::_global2;
const dll3::Global3& g3ref = dll3::_global3;

struct MainGlobal
{
    MainGlobal() { 
        dll1::_global1.rename("MainGlobal"); 
    };
};
MainGlobal _mainGlobal;


int main()
{
    for (const auto& s : dll1::_global1._tags) {
        std::cout << s << "\n";
    }
    std::cout << dll1::_global1._tag << "\n";
    //assert(dll1::_global1._tags.size() == 4);
    return 0;
}
