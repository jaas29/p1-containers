#include <containers/vector.hpp>
#include <containers/hash_map.hpp>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <utility> // std::move

// A suite that prints is not a suite that tests: reading output with your eyes
// is not a check, and a run that always exits 0 can never fail a build. This is
// the whole of the testing framework - no GoogleTest, no dependency to pin.
// Count what ran, report what failed, and exit nonzero if anything did.
static int checks_run = 0;
static int checks_failed = 0;

#define CHECK_EQ(actual, expected)                                             \
    do                                                                         \
    {                                                                          \
        ++checks_run;                                                          \
        if (!((actual) == (expected)))                                         \
        {                                                                      \
            ++checks_failed;                                                   \
            std::cout << "FAIL  " << __FILE__ << ":" << __LINE__ << "  "       \
                      << #actual << "\n        expected " << (expected)        \
                      << ", got " << (actual) << "\n";                         \
        }                                                                      \
    } while (false)

#define CHECK(condition) CHECK_EQ(static_cast<bool>(condition), true)

int main()
{
    // --- Build a vector -----------------------------------------------------
    containers::Vector<int> a;
    for (int i = 0; i < 5; ++i)
    {
        a.push_back(i * 10);
        std::cout << "size " << a.size() << "  cap " << a.capacity() << "\n";
    }
    std::cout << a[4] << "\n";
    CHECK_EQ(a.size(), 5u);
    CHECK_EQ(a.capacity(), 8u); // doubling: 1, 2, 4, 8
    CHECK_EQ(a[4], 40);

    // --- Copy constructor ---------------------------------------------------
    containers::Vector<int> b = a;
    b[0] = 999;
    std::cout << "a[0] = " << a[0] << "   b[0] = " << b[0] << "\n"; // changing b[0] must not change a[0]
    CHECK_EQ(a[0], 0);   // the copy is deep: writing b must not reach a
    CHECK_EQ(b[0], 999);

    // --- Copy assignment ----------------------------------------------------
    containers::Vector<int> c;
    c.push_back(7); // c now owns a 1-element buffer of its own
    c = a;          // copy assignment: that buffer must be released
    std::cout << "c[0] = " << c[0] << "  size " << c.size()
              << "  cap " << c.capacity() << "\n";

    containers::Vector<int> &alias = c;
    c = alias; // self-assignment, invisible to the compiler
    std::cout << "after self-assign c[3] = " << c[3] << "\n";
    CHECK_EQ(c[3], 30); // self-assignment must not destroy the buffer
    CHECK_EQ(c.size(), 5u);

    // --- Move assignment ----------------------------------------------------
    containers::Vector<int> m;
    m.push_back(1);
    m = std::move(b); // move-assign onto a vector that already owns a buffer
    std::cout << "after move-assign m[0] = " << m[0] << "  size " << m.size()
              << "  cap " << m.capacity() << "\n";
    std::cout << "moved-from b: size " << b.size() << "  cap " << b.capacity() << "\n";
    CHECK_EQ(m.size(), 5u);
    CHECK_EQ(b.size(), 0u); // a moved-from vector owns nothing
    CHECK_EQ(b.capacity(), 0u);

    b.push_back(7); // a moved-from vector must still be usable
    std::cout << "reused b: b[0] = " << b[0] << "  size " << b.size() << "\n";

    containers::Vector<int> &self = m;
    m = std::move(self); // self-move: the guard is load-bearing here
    std::cout << "after self-move m[0] = " << m[0] << "  size " << m.size() << "\n";
    CHECK_EQ(m[0], 999); // the self-move guard is load-bearing here

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
    CHECK_EQ(outer[2][3], 23);  // deep one level down, with an owning T
    CHECK_EQ(ocopy[2][3], 999);

    containers::Vector<int> v;
    for (int x : {50, 20, 40, 10, 30})
        v.push_back(x);

    std::sort(v.begin(), v.end());
    for (int x : v)
        std::cout << x << " "; // range-for, see below
    std::cout << "\n";

    auto it = std::find(v.begin(), v.end(), 40);
    std::cout << "found at index " << (it - v.begin()) << "\n";
    CHECK_EQ(v[0], 10); // std::sort ran on our own buffer
    CHECK_EQ(v[4], 50);
    CHECK_EQ(it - v.begin(), 3);
    std::cout << "sum " << std::accumulate(v.begin(), v.end(), 0) << "\n";

    const containers::Vector<int> &cv = v; // the const path
    std::cout << "cv[0] " << cv[0]
              << "  count " << std::count(cv.begin(), cv.end(), 30) << "\n";
    CHECK_EQ(std::accumulate(v.begin(), v.end(), 0), 150);
    CHECK_EQ(cv[0], 10); // the const path: const operator[] and const begin/end
    CHECK_EQ(std::count(cv.begin(), cv.end(), 30), 1);

    containers::HashMap<std::string, int> ages;
    ages.insert("jose", 21);
    ages.insert("roy", 45);
    ages.insert("ada", 36);
    ages.insert("grace", 85);
    ages.insert("alan", 41);
    ages.insert("jose", 22); // overwrite - must NOT become a sixth entry

    if (auto found = ages.find("jose"))
        std::cout << "jose -> " << *found << "\n";
    if (!ages.find("nobody"))
        std::cout << "nobody -> not found\n";

    // Six inserts, five keys. The print above only proves the value changed;
    // this proves the overwrite did not quietly append a sixth entry.
    CHECK_EQ(ages.size(), 5u);
    CHECK_EQ(*ages.find("jose"), 22);
    CHECK(!ages.find("nobody"));

    // --- Rehashing ----------------------------------------------------------
    // The table must actually grow, and every key must survive the rebuild.
    // A rehash that relocated chains without re-asking each key would still
    // report a plausible bucket count and load factor, and lose most keys.
    containers::HashMap<std::string, int> grown;
    for (int i = 0; i < 40; ++i)
        grown.insert("key" + std::to_string(i), i);

    std::cout << "after 40 inserts: size " << grown.size()
              << "  buckets " << grown.bucket_count()
              << "  load " << grown.load_factor() << "\n";

    CHECK_EQ(grown.size(), 40u);
    CHECK(grown.bucket_count() > 8u);                                 // it grew
    // Bound to a local first: the comma inside the template argument list
    // would otherwise be read as a macro argument separator.
    const double ceiling = containers::HashMap<std::string, int>::max_load_factor;
    CHECK(grown.load_factor() <= ceiling);

    int found_after_rehash = 0;
    for (int i = 0; i < 40; ++i)
        if (auto hit = grown.find("key" + std::to_string(i)); hit && *hit == i)
            ++found_after_rehash;
    CHECK_EQ(found_after_rehash, 40);

    // --- Rehashing with a V that owns memory --------------------------------
    // Record 0009: at V = int a copy and a move compile to the same thing and
    // there is no owned buffer to double-free, so an int-valued map cannot see
    // whether rehash relocates correctly. This is the test that can.
    containers::HashMap<std::string, containers::Vector<int>> owning;
    for (int i = 0; i < 40; ++i)
    {
        containers::Vector<int> row;
        for (int j = 0; j < 5; ++j)
            row.push_back(i * 100 + j);
        owning.insert("key" + std::to_string(i), row);
    }
    int intact = 0;
    for (int i = 0; i < 40; ++i)
    {
        auto row = owning.find("key" + std::to_string(i));
        if (row && (*row).size() == 5u && (*row)[3] == i * 100 + 3)
            ++intact;
    }
    std::cout << "owning values intact after rehashing: " << intact << " / 40\n";
    CHECK_EQ(intact, 40);

    // --- Iterating a bucketed structure -------------------------------------
    containers::HashMap<std::string, int> walk;
    for (int i = 0; i < 10; ++i)
        walk.insert("key" + std::to_string(i), i);

    int visited = 0;
    int sum = 0;
    for (auto &entry : walk)
    {
        ++visited;
        sum += entry.value;
    }
    std::cout << "iteration visited " << visited << " entries, sum " << sum << "\n";
    // The sum is the real check. A settle() that skipped a non-empty bucket
    // would still print a plausible list of entries.
    CHECK_EQ(visited, 10);
    CHECK_EQ(sum, 45);

    for (auto &entry : walk) // operator* returns a reference, so this writes through
        entry.value *= 100;
    CHECK_EQ(*walk.find("key7"), 700);

    // The five iterator_traits typedefs are what make this compile at all.
    CHECK_EQ(std::count_if(walk.begin(), walk.end(),
                           [](const auto &e) { return e.value > 400; }),
             5);
    CHECK_EQ(std::distance(walk.begin(), walk.end()), 10);

    // --- erase --------------------------------------------------------------
    CHECK(walk.erase("key3"));  // present
    CHECK(!walk.erase("key3")); // and now it is not
    CHECK_EQ(walk.size(), 9u);
    CHECK(!walk.find("key3"));

    visited = 0;
    for (auto &entry : walk)
    {
        (void)entry;
        ++visited;
    }
    CHECK_EQ(visited, 9);

    // --- The empty map ------------------------------------------------------
    // Nothing was written to make this true; it falls out of settle() running
    // off the end of the table.
    containers::HashMap<std::string, int> empty;
    CHECK(empty.begin() == empty.end());
    visited = 0;
    for (auto &entry : empty)
    {
        (void)entry;
        ++visited;
    }
    CHECK_EQ(visited, 0);

    // --- Result -------------------------------------------------------------
    std::cout << "\n"
              << (checks_run - checks_failed) << " / " << checks_run
              << " checks passed\n";
    return checks_failed == 0 ? 0 : 1;
} // every destructor runs here: a, b, c, m (empty), n
