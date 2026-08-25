#include <containers/vec.hpp>
#include <iostream>

int main()
{
    containers::Buffer b(4);
    for (std::size_t i = 0; i < b.size(); ++i)
        b[i] = static_cast<int>(i * i);
    std::cout << b[3] << '\n';
}