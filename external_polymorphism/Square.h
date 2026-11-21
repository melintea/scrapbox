//---- <Square.h> ----------------

#pragma once

class Square
{
    public:

        explicit Square( double side )
            : side_( side )
        {
            /* Checking that the given side length is valid */
        }

        double side() const { return side_; }

        /* Several more getters and square-specific utility functions */

    private:
    
        double side_;
        /* Several more data members */
};

