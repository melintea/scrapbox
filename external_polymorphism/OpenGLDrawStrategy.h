//---- <OpenGLDrawStrategy.h> ----------------
#include <Circle.h>
#include <Square.h>
//#include /* OpenGL graphics library headers */

#pragma once


class OpenGLDrawStrategy
{
    public:

        explicit OpenGLDrawStrategy( /* Drawing related arguments */ );
        void operator()( Circle const& circle ) const;
        void operator()( Square const& square ) const;

    private:
    
        /*Drawing related data members, e.g. colors, textures, ... */
};

