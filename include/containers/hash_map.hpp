#pragma once
#include <containers/vector.hpp>
#include <cstddef>
#include <functional> // std::hash
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
        }

        std::optional<V> find(const K &key) const
        {
            const Vector<Entry> &chain = m_buckets[bucket_for(key)];
            for (std::size_t i = 0; i < chain.size(); ++i)
                if (chain[i].key == key)
                    return chain[i].value;
            return std::nullopt;
        }
    };
}