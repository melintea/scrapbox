//

#include <string>
#include <vector>

#include "dll1.hpp"

namespace dll2 {


struct Global2
{
    Global2() { 
        dll1::_global1.rename("Global2"); 
    };
};

extern Global2 _global2;

} // dll2
