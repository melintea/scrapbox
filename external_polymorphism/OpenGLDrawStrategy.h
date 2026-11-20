//---- <OpenGLDrawStrategy.h> ----------------
#include <Circle>
#include <Square>
#include /* OpenGL graphics library headers */

class OpenGLDrawStrategy
{
    public:

        explicit OpenGLDrawStrategy( /* Drawing related arguments */ );
        void operator()( Circle const& circle ) const;
        void operator()( Square const& square ) const;

    private:
    
        /*Drawing related data members, e.g. colors, textures, ... */
};

