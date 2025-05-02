//

#include <string>
#include <vector>

#include "dll1.hpp"

namespace dll3 {


struct Global3
{
    Global3() { 
        dll1::_global1.rename("Global3"); 
    };
};

extern Global3 _global3;

} // dll3
