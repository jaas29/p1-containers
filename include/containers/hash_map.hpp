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
    };
}