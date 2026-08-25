#pragma once
#include <cstddef>

namespace containers
{

    class Buffer
    {
    public:
        explicit Buffer(std::size_t n) : data_(new int[n]), size_(n) {}
        ~Buffer() { delete[] data_; }

        int &operator[](std::size_t i) { return data_[i]; }
        std::size_t size() const { return size_; }

    private:
        int *data_;
        std::size_t size_;
    };

} // namespace containers