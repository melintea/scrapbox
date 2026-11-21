#include <Circle.h>
#include <Square.h>
#include <Shape.h>
#include <OpenGLDrawStrategy.h>
#include <memory>
#include <vector>

int draw( int i )
{
    // ... drawing an int, for instance by printing it to the command line
    return 0;
}

int main()
{
    using Shapes = std::vector<std::unique_ptr<ShapeConcept>>;
    using CircleModel = ShapeModel<Circle,OpenGLDrawStrategy>;
    using SquareModel = ShapeModel<Square,OpenGLDrawStrategy>;

    Shapes shapes{};
    
    shapes.emplace_back(
                std::make_unique<CircleModel>(
                    Circle{2.3}, OpenGLDrawStrategy(/*...red...*/) ) );
    shapes.emplace_back(
            std::make_unique<SquareModel>(
                Square{1.2}, OpenGLDrawStrategy(/*...green...*/) ) );
    shapes.emplace_back(
            std::make_unique<CircleModel>(
                Circle{4.1}, OpenGLDrawStrategy(/*...blue...*/) ) );
    shapes.emplace_back( std::make_unique<ShapeModel<int>>( 42 ) );
    
    for( auto const& shape : shapes )
    {
        shape->draw();
    }
    
    return EXIT_SUCCESS;
}

