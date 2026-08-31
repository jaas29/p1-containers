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
} //~vector a; // destructor is called here, freeing the allocated memory