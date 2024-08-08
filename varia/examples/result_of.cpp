
#include <iostream>
#include <source_location>
#include <type_traits>

struct S
{
    double operator()(char, int&);
    float operator()(int) { return 1.0; }
};

template<class T>
typename std::result_of<T(int)>::type func(T& t)
{
    std::cout << "overload of f for callable T\n";
    return t(0);
}

template<class T, class U>
int func(U u)
{
    std::cout << "overload of f for non-callable T: u=" << u << '\n';
    return u;
}

int main()
{
    // the result of invoking S with char and int& arguments is double
    std::result_of<S(char, int&)>::type d = 3.14; // d has type double
    static_assert(std::is_same<decltype(d), double>::value, "");
    std::cout << "d=" << d << '\n';

    // std::invoke_result uses different syntax (no parentheses)
    std::invoke_result<S,char,int&>::type b = 3.14;
    static_assert(std::is_same<decltype(b), double>::value, "");
    std::cout << "b=" << b << '\n';

    // the result of invoking S with int argument is float
    std::result_of<S(int)>::type x = 3.14; // x has type float
    static_assert(std::is_same<decltype(x), float>::value, "");
    std::cout << "x=" << x << '\n';

    // result_of can be used with a pointer to member function as follows
    struct C { double Func(char, int&); };
    std::result_of<decltype(&C::Func)(C, char, int&)>::type g = 3.14;
    static_assert(std::is_same<decltype(g), double>::value, "");
    std::cout << "g=" << g << '\n';

    func<C>(1); // may fail to compile in C++11; calls the non-callable overload in C++14
}

