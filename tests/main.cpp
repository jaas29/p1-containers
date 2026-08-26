#include <containers/vector.hpp>
#include <iostream>

int main()
{
    containers::Vector v;
    for (int i = 0; i < 5; ++i)
    {
        v.push_back(i * 10);
        std::cout << "size " << v.size() << "  cap " << v.capacity() << "\n";
    }
    std::cout << v[3] << "\n";
}