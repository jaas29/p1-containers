// Benchmarks for mystl, against the standard library equivalents.
//
// BUILD THIS UNDER THE release PRESET ONLY:
//     cmake --preset release && cmake --build build/release && ./build/release/bench
//
// The dev and asan presets pass no -O flag. Under dev this same benchmark reports
// that our Vector beats std::vector, which is false: at -O0 nothing inlines, so a
// deep stack of thin abstractions pays a real call per layer while a three-member
// class pays almost nothing. An unoptimized benchmark measures abstraction depth,
// not container speed. See learning record 0010.

#include <containers/hash_map.hpp>
#include <containers/vector.hpp>

#include <chrono>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <vector>

using clk = std::chrono::steady_clock; // monotonic; system_clock can step backwards

// The optimizer may delete any computation whose result nobody observes. Writing
// into a volatile forces it to assume the value is observable from outside the
// program, so the work feeding it has to actually happen.
volatile std::size_t sink = 0;

// Median of 5 runs, in milliseconds. The first run pays for cold caches, and any
// run can be interrupted by the scheduler; the median discards one outlier in
// either direction where a mean would absorb it.
template <typename F>
static double time_once(F &&f)
{
    auto a = clk::now();
    f();
    auto b = clk::now();
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

static double median_ms(const std::function<void()> &f, int trials = 7)
{
    f(); // warm-up
    std::vector<double> t;
    for (int i = 0; i < trials; ++i)
        t.push_back(time_once(f));
    return median(t);
}

struct Result
{
    double mine, theirs, ratio, lo, hi;
};

// Time two implementations against each other honestly.
//
// The first version of this ran all N trials of one implementation and then all
// N of the other. That is the bug that made the lookup-hit ratio swing between
// 0.85x and 1.34x across runs: a median of five protects against a scheduler
// hiccup WITHIN a block, but if the machine warms up or a background process
// starts between the two blocks, the whole disturbance lands on one side and
// the ratio is measuring the machine, not the container.
//
// So the trials are INTERLEAVED - mine, theirs, mine, theirs - and both see the
// same conditions in the same trial. The ratio is reported with its range across
// trials, because a single ratio with no spread is a claim you cannot defend.
template <typename A, typename B>
static Result compare(A &&a, B &&b, int trials = 11)
{
    a(); // warm-up, untimed: the first trial otherwise pays for cold caches
    b(); // and lazy page faults, which is not what we are measuring

    std::vector<double> ta, tb, tr;
    for (int i = 0; i < trials; ++i)
    {
        double x = time_once(a);
        double y = time_once(b);
        ta.push_back(x);
        tb.push_back(y);
        tr.push_back(x / y);
    }
    std::sort(tr.begin(), tr.end());
    return {median(ta), median(tb), median(tr), tr.front(), tr.back()};
}

static void row(const char *label, const Result &r)
{
    // A verdict only where the range does not straddle 1.0. If it does, the
    // honest word is "parity" - the difference is smaller than the noise.
    const char *verdict = (r.lo > 1.0) ? "std wins"
                          : (r.hi < 1.0) ? "MINE wins"
                                         : "parity";
    std::printf("%-30s %8.2f %8.2f %7.2fx  [%.2f-%.2f]  %s\n",
                label, r.mine, r.theirs, r.ratio, r.lo, r.hi, verdict);
}

// --- Counting, where timing cannot answer -----------------------------------
//
// Preallocating the buckets shows how much of insert is rehashing, but the rest
// is per-insert work that a stopwatch cannot break down. So stop timing and start
// counting: a key type that records every operation performed on it.
struct Counted
{
    std::string s;
    static inline int copies = 0, moves = 0, defaults = 0;

    Counted() { ++defaults; }
    explicit Counted(std::string v) : s(std::move(v)) {}
    Counted(const Counted &o) : s(o.s) { ++copies; }
    Counted(Counted &&o) noexcept : s(std::move(o.s)) { ++moves; }
    Counted &operator=(const Counted &o) { s = o.s; ++copies; return *this; }
    Counted &operator=(Counted &&o) noexcept { s = std::move(o.s); ++moves; return *this; }
    bool operator==(const Counted &o) const { return s == o.s; }
};

template <>
struct std::hash<Counted>
{
    std::size_t operator()(const Counted &c) const { return std::hash<std::string>{}(c.s); }
};

static void count_key_operations()
{
    const int n = 100;
    std::printf("\nkey operations per %d inserts:\n", n);
    std::printf("  %-22s %8s %8s %10s\n", "", "copies", "moves", "defaults");

    {
        containers::HashMap<Counted, int> m;
        Counted::copies = Counted::moves = Counted::defaults = 0;
        for (int i = 0; i < n; ++i)
            m.insert(Counted("key" + std::to_string(i)), i);
        std::printf("  %-22s %8d %8d %10d\n", "mine", Counted::copies, Counted::moves, Counted::defaults);
    }
    {
        std::unordered_map<Counted, int> m;
        Counted::copies = Counted::moves = Counted::defaults = 0;
        for (int i = 0; i < n; ++i)
            m.insert({Counted("key" + std::to_string(i)), i});
        std::printf("  %-22s %8d %8d %10d\n", "std::unordered_map", Counted::copies, Counted::moves, Counted::defaults);
    }
}

int main()
{
    const int N = 200000;

    // Build the keys once, outside every timed block, so we measure the containers
    // and not std::to_string.
    std::vector<std::string> keys;
    keys.reserve(N);
    for (int i = 0; i < N; ++i)
        keys.push_back("key" + std::to_string(i));

    std::vector<std::string> absent;
    absent.reserve(N);
    for (int i = 0; i < N; ++i)
        absent.push_back("missing" + std::to_string(i));

    std::printf("mystl vs the standard library - %d operations, 11 interleaved trials\n", N);
    std::printf("ratio = mine / std. Above 1.00 the standard library wins.\n");
    std::printf("A range straddling 1.00 means the difference is inside the noise.\n\n");
    std::printf("%-30s %8s %8s %8s  %11s  %s\n",
                "operation", "mine(ms)", "std(ms)", "ratio", "range", "verdict");
    std::printf("%-30s %8s %8s %8s  %11s  %s\n",
                "------------------------------", "--------", "-------", "-------",
                "-----------", "-------");

    // --- Vector -------------------------------------------------------------
    row("vector push_back, int",
        compare(
            [&] { containers::Vector<int> v; for (int i = 0; i < N; ++i) v.push_back(i); sink += v.size(); },
        [&] { std::vector<int>        v; for (int i = 0; i < N; ++i) v.push_back(i); sink += v.size(); }));

    // --- HashMap insert -----------------------------------------------------
    row("hashmap insert, string keys",
        compare(
            [&] { containers::HashMap<std::string, int> m; for (int i = 0; i < N; ++i) m.insert(keys[i], i); sink += m.size(); },
        [&] { std::unordered_map<std::string, int>  m; for (int i = 0; i < N; ++i) m.insert({keys[i], i}); sink += m.size(); }));

    // --- HashMap lookup -----------------------------------------------------
    // Built once, outside the timing, so we measure lookup and not construction.
    containers::HashMap<std::string, int> mine;
    std::unordered_map<std::string, int> theirs;
    for (int i = 0; i < N; ++i)
    {
        mine.insert(keys[i], i);
        theirs.insert({keys[i], i});
    }

    row("hashmap lookup, all hits",
        compare(
            [&] { std::size_t s = 0; for (int i = 0; i < N; ++i) if (auto v = mine.find(keys[i])) s += *v; sink += s; },
        [&] { std::size_t s = 0; for (int i = 0; i < N; ++i) { auto it = theirs.find(keys[i]); if (it != theirs.end()) s += it->second; } sink += s; }));

    row("hashmap lookup, all misses",
        compare(
            [&] { std::size_t s = 0; for (int i = 0; i < N; ++i) if (mine.find(absent[i])) ++s; sink += s; },
        [&] { std::size_t s = 0; for (int i = 0; i < N; ++i) if (theirs.find(absent[i]) != theirs.end()) ++s; sink += s; }));

    // --- Where the insert time goes ----------------------------------------
    // Rehashing is the obvious suspect. Isolate it by handing the map every
    // bucket it will need up front, so no rehash ever fires.
    std::printf("\nisolating the insert cost:\n");
    double growing = median_ms([&] { containers::HashMap<std::string, int> m;          for (int i = 0; i < N; ++i) m.insert(keys[i], i); sink += m.size(); });
    double preallocated = median_ms([&] { containers::HashMap<std::string, int> m(524288); for (int i = 0; i < N; ++i) m.insert(keys[i], i); sink += m.size(); });
    std::printf("  mine, growing from 8       : %7.2f ms\n", growing);
    std::printf("  mine, buckets preallocated : %7.2f ms   <- rehashing is %.0f%% of it\n",
                preallocated, 100.0 * (growing - preallocated) / growing);

    std::printf("\nfinal table state: %zu entries in %zu buckets, load factor %.3f\n",
                mine.size(), mine.bucket_count(), mine.load_factor());

    count_key_operations();
    return 0;
}
