#pragma once
#include <cstddef>

namespace containers
{
    // A growable array of ints, built the way std::vector is built internally.
    //
    // The whole type is three pieces of state:
    //   m_data     - the address of a heap buffer (or nullptr if we own nothing)
    //   m_size     - how many values the caller has actually stored
    //   m_capacity - how many slots that buffer has room for
    //
    // The invariant that must hold after every operation: m_size <= m_capacity.
    class Vector
    {
        int *m_data = nullptr;      // Pointer to the dynamically allocated array
        std::size_t m_size = 0;     // Current number of elements in the vector
        std::size_t m_capacity = 0; // Current capacity of the vector

    public:
        // Append one value to the end.
        void push_back(int value)
        {
            // No room left, so buy more before writing. On the very first call
            // m_size and m_capacity are both 0, so this fires immediately and
            // allocates the first buffer.
            if (m_size == m_capacity)
                grow();

            m_data[m_size] = value; // write into the first free slot
            ++m_size;               // exactly one increment: one push, one element
        }

        int &operator[](std::size_t index)
        {
            return m_data[index];
        }

        std::size_t size() const
        {
            return m_size;
        }

        std::size_t capacity() const
        {
            return m_capacity;
        }

    public:
        // Default constructor initializes an empty vector
        Vector() = default;
        // Destructor to free the allocated memory when the Vector object is destroyed
        ~Vector()
        {
            delete[] m_data; // Free the allocated memory
        }
        // Delete copy constructor and copy assignment operator to prevent copying
        Vector(const Vector &) = delete;
        Vector &operator=(const Vector &) = delete;

    private:
        // Replace the current buffer with one twice as large.
        // A heap buffer cannot be resized in place, because the memory directly
        // after it may already belong to something else. So: allocate a new one,
        // copy across, release the old one, adopt the new one. In that order.
        void grow()
        {
            // Doubling, not +1. Copying is the expensive part, so doubling makes
            // 0 is a special case because doubling zero is still zero.
            std::size_t new_capacity = (m_capacity == 0) ? 1 : m_capacity * 2;

            int *fresh = new int[new_capacity]; // 1. bigger buffer

            for (std::size_t i = 0; i < m_size; ++i) // 2. copy the old values
                fresh[i] = m_data[i];

            delete[] m_data; // 3. release the old buffer
            //    (delete[] on nullptr is
            //     defined and does nothing,
            //     so the first grow is safe)

            m_data = fresh;            // 4. adopt the new one
            m_capacity = new_capacity; //    and record its size
        }
    };

} // namespace containers
