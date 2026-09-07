#include <containers/vector.hpp>
#include <iostream>
#include <utility> // std::move

int main()
{
    // --- Build a vector -----------------------------------------------------
    containers::Vector<int> a;
    for (int i = 0; i < 5; ++i)
    {
        a.push_back(i * 10);
        std::cout << "size " << a.size() << "  cap " << a.capacity() << "\n";
    }
    std::cout << a[3] << "\n";

    // --- Copy constructor ---------------------------------------------------
    containers::Vector<int> b = a;
    b[0] = 999;
    std::cout << "a[0] = " << a[0] << "   b[0] = " << b[0] << "\n";

    // --- Copy assignment ----------------------------------------------------
    containers::Vector<int> c;
    c.push_back(7); // c now owns a 1-element buffer of its own
    c = a;          // copy assignment: that buffer must be released
    std::cout << "c[0] = " << c[0] << "  size " << c.size()
              << "  cap " << c.capacity() << "\n";

    containers::Vector<int> &alias = c;
    c = alias; // self-assignment, invisible to the compiler
    std::cout << "after self-assign c[3] = " << c[3] << "\n";

    // --- Move assignment ----------------------------------------------------
    containers::Vector<int> m;
    m.push_back(1);
    m = std::move(b); // move-assign onto a vector that already owns a buffer
    std::cout << "after move-assign m[0] = " << m[0] << "  size " << m.size()
              << "  cap " << m.capacity() << "\n";
    std::cout << "moved-from b: size " << b.size() << "  cap " << b.capacity() << "\n";

    b.push_back(7); // a moved-from vector must still be usable
    std::cout << "reused b: b[0] = " << b[0] << "  size " << b.size() << "\n";

    containers::Vector<int> &self = m;
    m = std::move(self); // self-move: the guard is load-bearing here
    std::cout << "after self-move m[0] = " << m[0] << "  size " << m.size() << "\n";

    // --- Move constructor ---------------------------------------------------
    containers::Vector<int> n = std::move(m);
    std::cout << "move-constructed n[0] = " << n[0] << "  size " << n.size() << "\n";

    // --- T that owns memory: the case Vector<int> cannot test ------------
    containers::Vector<containers::Vector<int>> outer;
    for (int r = 0; r < 3; ++r)
    {
        containers::Vector<int> row;
        for (int i = 0; i < 4; ++i)
            row.push_back(r * 10 + i);
        outer.push_back(row);
    }
    std::cout << "outer[2][3] = " << outer[2][3] << "  size " << outer.size() << "\n";

    containers::Vector<containers::Vector<int>> &oalias = outer;
    outer = oalias; // self-assign when T has its own copy assignment
    std::cout << "after self-assign outer[2][3] = " << outer[2][3] << "\n";

    containers::Vector<containers::Vector<int>> ocopy = outer;
    ocopy[2][3] = 999; // deep copy must be independent one level down
    std::cout << "outer[2][3] = " << outer[2][3]
              << "   ocopy[2][3] = " << ocopy[2][3] << "\n";

    return 0;
} // every destructor runs here: a, b, c, m (empty), n
