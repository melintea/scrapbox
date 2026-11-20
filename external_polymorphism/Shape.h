//---- <Shape.h> ----------------
#include <functional>
#include <stdexcept>
#include <utility>

class ShapeConcept
{
    public:

        virtual ~ShapeConcept() = default;
        virtual void draw() const = 0;// ... Potentially more polymorphic operations
};
template< typename ShapeT >
class ShapeModel : public ShapeConcept
{
    public:

        using DrawStrategy = std::function<void(ShapeT const&)>;

        explicit ShapeModel( ShapeT shape, DrawStrategy drawer )
            : shape_{ std::move(shape) }
        , drawer_{ std::move(drawer) }
        {
            /* Checking that the given 'std::function' is not empty */
        }

        void draw() const override { drawer_(shape_); }

        // ... Potentially more polymorphic operations

    private:

        ShapeT shape_;
        DrawStrategy drawer_;
};


struct DefaultDrawer
{
    template< typename T >
        void operator()( T const& obj ) const {
            draw(obj);
        }
};

template< typename ShapeT
, typename DrawStrategy = DefaultDrawer >
class ShapeModel : public ShapeConcept{

    public:
    
        explicit ShapeModel( ShapeT shape, DrawStrategy drawer = DefaultDrawer{} )
            // ... as before
};


