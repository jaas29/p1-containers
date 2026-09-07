#include <containers/vector.hpp>
#include <iostream>

int main()
{
    containers::Vector a;
    for (int i = 0; i < 5; ++i)
    {
        a.push_back(i * 10);
        std::cout << "size " << a.size() << "  cap " << a.capacity() << "\n";
    }
    std::cout << a[3] << "\n";
    containers::Vector b = a; // copy constructor
    b[0] = 999;
    std::cout << "a[0] = " << a[0] << "   b[0] = " << b[0] << "\n";
    containers::Vector c;
    c.push_back(7); // c now owns a 1-element buffer of its own
    c = a;          // copy assignment: that buffer must be released
    std::cout << "c[0] = " << c[0] << "  size " << c.size()
              << "  cap " << c.capacity() << "\n";
    containers::Vector &alias = c;
    c = alias;                                                // self-assignment, invisible to the compiler
    std::cout << "after self-assign c[3] = " << c[3] << "\n"; // 30

} //~vector a; // destructor is called here, freeing the allocated memory