#pragma once
#include <containers/vector.hpp>
#include <cstddef>
#include <functional> // std::hash
#include <iterator>  // std::forward_iterator_tag
#include <optional>

namespace containers
{
    template <typename K, typename V>
    class HashMap
    {
        struct Entry
        {
            K key{}; // the braces matter - see the note below
            V value{};
        };

        Vector<Vector<Entry>> m_buckets; // one chain per bucket
        std::size_t m_size = 0;          // total pairs stored, not buckets used

    public:
        explicit HashMap(std::size_t bucket_count = 8)
        {
            for (std::size_t i = 0; i < bucket_count; ++i)
                m_buckets.push_back(Vector<Entry>{});
        }

        std::size_t size() const { return m_size; }
        std::size_t bucket_count() const { return m_buckets.size(); }

        std::size_t bucket_for(const K &key) const
        {
            return std::hash<K>{}(key) % m_buckets.size();
        }

        void insert(const K &key, const V &value)
        {
            Vector<Entry> &chain = m_buckets[bucket_for(key)];

            for (std::size_t i = 0; i < chain.size(); ++i) // already present?
                if (chain[i].key == key)
                {
                    chain[i].value = value; // overwrite
                    return;                 // m_size unchanged
                }

            chain.push_back(Entry{key, value});
            ++m_size;

            if (load_factor() > max_load_factor)
                rehash(m_buckets.size() * 2);
        }

        std::optional<V>
        find(const K &key) const
        {
            const Vector<Entry> &chain = m_buckets[bucket_for(key)];
            for (std::size_t i = 0; i < chain.size(); ++i)
                if (chain[i].key == key)
                    return chain[i].value;
            return std::nullopt;
        }
        static constexpr double max_load_factor = 0.75;

        double load_factor() const
        {
            return static_cast<double>(m_size) / static_cast<double>(m_buckets.size());
        }

        void rehash(std::size_t new_bucket_count)
        {
            Vector<Vector<Entry>> fresh; // 1. build the new table
            for (std::size_t i = 0; i < new_bucket_count; ++i)
                fresh.push_back(Vector<Entry>{});

            for (std::size_t b = 0; b < m_buckets.size(); ++b) // 2. re-ask every key
            {
                Vector<Entry> &chain = m_buckets[b];
                for (std::size_t i = 0; i < chain.size(); ++i)
                {
                    std::size_t nb = std::hash<K>{}(chain[i].key) % new_bucket_count;
                    fresh[nb].push_back(std::move(chain[i])); // move, not copy
                }
            }

            m_buckets = std::move(fresh); // 3. adopt it
        }
        class iterator
        {
            Vector<Vector<Entry>> *m_buckets = nullptr;
            std::size_t m_bucket = 0; // which bucket
            std::size_t m_index = 0;  // how far down its chain

            // Leave the position on a real entry, or exactly on the end position
            // (m_bucket == bucket count). Those are the only two legal states,
            // so this is called after every move and on construction.
            void settle()
            {
                while (m_bucket < m_buckets->size() &&
                       m_index >= (*m_buckets)[m_bucket].size())
                {
                    ++m_bucket;   // this chain is exhausted, try the next bucket
                    m_index = 0;
                }
            }

        public:
            // What <algorithm> looks up through std::iterator_traits. Declarations
            // only - they change nothing at runtime, they make the iterator legible.
            using iterator_category = std::forward_iterator_tag;
            using value_type = Entry;
            using difference_type = std::ptrdiff_t;
            using pointer = Entry *;
            using reference = Entry &;

            iterator() = default; // forward iterators must be default-constructible

            iterator(Vector<Vector<Entry>> *buckets, std::size_t bucket, std::size_t index)
                : m_buckets(buckets), m_bucket(bucket), m_index(index)
            {
                settle(); // enforce the invariant on construction
            }

            Entry &operator*() const { return (*m_buckets)[m_bucket][m_index]; }
            Entry *operator->() const { return &(*m_buckets)[m_bucket][m_index]; }

            iterator &operator++()
            {
                ++m_index;
                settle();
                return *this;
            }

            // Post-increment: returns the old position, so prefer ++it in loops.
            iterator operator++(int)
            {
                iterator before = *this;
                ++(*this);
                return before;
            }

            bool operator==(const iterator &other) const
            {
                return m_bucket == other.m_bucket && m_index == other.m_index;
            }
            bool operator!=(const iterator &other) const { return !(*this == other); }
        };

        iterator begin() { return iterator(&m_buckets, 0, 0); }
        iterator end() { return iterator(&m_buckets, m_buckets.size(), 0); }
        bool erase(const K &key)
        {
            Vector<Entry> &chain = m_buckets[bucket_for(key)];
            for (std::size_t i = 0; i < chain.size(); ++i)
                if (chain[i].key == key)
                {
                    chain[i] = std::move(chain[chain.size() - 1]); // pull the last one back
                    chain.pop_back();                              // drop the tail
                    --m_size;
                    return true;
                }
            return false;
        }
    };
}