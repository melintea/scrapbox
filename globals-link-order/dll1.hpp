//

#include <string>
#include <vector>

#pragma once

namespace dll1 {


struct Global1
{
    std::string _tag;
    std::vector<std::string> _tags;

    Global1() { 
        _tag = "Global1"; 
	_tags.push_back("Global1");
    };
    
    void rename(const std::string& n) { 
        _tag += "-"; _tag += n; 
	_tags.push_back(n);
    }
};


extern Global1 _global1;


} // dll1
