# mystl vs the standard library

A measured comparison of `containers::Vector<T>` and `containers::HashMap<K,V>` against
`std::vector` and `std::unordered_map`, with an account of where the standard library wins,
where it does not, and why in both directions.

**Machine.** Apple M3 Pro, macOS 26.5, Apple clang 17.0.0.
**Build.** `cmake --preset release` (`-O3 -DNDEBUG`, no sanitizers).
**Workload.** 200,000 operations per measurement. String keys `key0`…`key199999`.
**Reproduce.** `cmake --preset release && cmake --build build/release && ./build/release/bench`

---

## 1. How these numbers were produced, and one way they were wrong first

The first version of this harness reported that `HashMap::find` beat `std::unordered_map`
on hits by 10%. That result was not real, and the way it failed is worth more than the
number was.

The harness timed each implementation as a block: five trials of mine, then five trials of
theirs, median of each. A median of five defends against a scheduler hiccup *within* a
block. It does nothing about the machine changing *between* blocks — CPU frequency ramping,
a background process starting, cache state left behind by the previous block. Any of those
lands entirely on one side, and the ratio then measures the machine rather than the
container. Run to run, the same comparison produced 0.85x, 1.34x, 1.04x, 1.26x, 0.94x.

Three changes fixed it:

- **Trials are interleaved** — mine, theirs, mine, theirs — so both implementations meet the
  same machine in the same trial. This is the change that mattered. Eleven *sequential*
  trials per side would still have been wrong.
- **An untimed warm-up pass**, so the first trial is not paying for cold caches and lazy
  page faults.
- **The ratio is reported with its range across trials**, and the verdict reads `parity`
  whenever that range straddles 1.00.

Every figure below is from the corrected harness. Where a range crosses 1.00 this report
claims nothing, because a single ratio with no spread is an anecdote rather than a
measurement.

---

## 2. Results

Ratio is mine ÷ std. Above 1.00 the standard library is faster.

| Operation | mine (ms) | std (ms) | ratio | range | verdict |
|---|---|---|---|---|---|
| `vector` push_back, `int` | 0.15 | 0.14 | 1.08x | 0.85–1.28 | parity |
| `hashmap` insert, string keys | 61.03 | 11.03 | **5.70x** | 5.2–6.3 | **std wins** |
| `hashmap` lookup, all hits | 4.73 | 5.28 | 0.95x | 0.85–1.18 | parity |
| `hashmap` lookup, all misses | 3.10 | 5.25 | **0.58x** | 0.53–0.62 | **mine wins** |

Verdicts held across four independent runs of the whole benchmark.

---

## 3. Where the standard library wins: insert, by 5.7x

This is the real gap, and it decomposes cleanly.

**Rehashing is 47% of it.** Hand the map every bucket it will need up front so no rehash
ever fires, and insert time drops from 73.42 ms to 38.97 ms. My `rehash` rebuilds the entire
table — every key is re-asked, because a key's bucket is `hash(key) % bucket_count` and
changing the bucket count changes every index. That work is inherent to the structure;
doing less of it means growing less often, not rehashing more cheaply.

**The remaining 3.5x is per-insert waste, and timing cannot see it.** So I stopped timing
and instrumented the key type to count operations instead:

| per 100 inserts | key copies | key moves | default-constructions |
|---|---|---|---|
| `containers::HashMap` | **391** | 87 | **390** |
| `std::unordered_map` | **100** | 100 | **0** |

Two distinct causes, both traceable to one line:

1. **~4 key copies per insert instead of 1.** `Entry{key, value}` copies the key into a
   temporary, then `push_back(const T&)` copies that `Entry` again into the slot. The
   standard library takes its pair by value and moves it into place, once.
2. **390 default-constructions nobody asked for.** `new T[n]` default-constructs *every*
   slot in every buffer — including empty buckets, and again in every fresh buffer on every
   growth. `std::unordered_map` performs exactly zero, because it never constructs an object
   it has not been asked to store.

**The root cause of both: `new T[n]` allocates *and constructs*, where the standard library
allocates raw bytes and constructs in place.** This is the same decision that makes
`Vector<T>` reject any `T` without a default constructor, and that stops `pop_back` from
destroying the element it removes. Three symptoms, one cause.

The fix is named and scoped — raw storage plus placement `new` plus explicit destructor
calls — and it is deliberately **not** in this submission. It touches every member of both
containers, and shipping it untested in the final week would have traded a measured,
explained gap for an unmeasured risk. Core Guideline
[Per.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rper-Knuth): this is a
deferral with a number attached rather than an omission.

---

## 4. Where mine wins: lookup misses, by about 1.7x

Consistent across every run, range never touching 1.00. Two reasons, and the second is the
interesting one.

**Shorter chains.** My `max_load_factor` is 0.75; libc++'s default is 1.0. Fewer entries per
bucket means fewer key comparisons per lookup.

**A cheaper modulo.** My bucket count is a power of two, so `hash % bucket_count` compiles to
a single bitmask. libc++ uses *prime* bucket counts, so its modulo is a real integer
division by a runtime value — one of the slowest integer operations on the machine.

**Why misses and not hits.** A hit stops at the matching key, on average halfway down the
chain. A miss walks the *whole* chain before it can conclude the key is absent. So shorter
chains pay off roughly twice as much on a miss, which is exactly the shape of the result:
clear win on misses, parity on hits.

---

## 5. The same win is also a real weakness

The power-of-two bucket count that wins above is a liability I can demonstrate.

`% 2^k` keeps only the low `k` bits of the hash. Any structure in those bits survives into
the bucket index. On this machine libc++'s `std::hash<int>` is the **identity function**
(measured: `42 -> 42`), so integer keys carry their structure directly into the bucket:

```
key 0  -> bucket 0     key 24 -> bucket 0
key 8  -> bucket 0     key 32 -> bucket 0
key 16 -> bucket 0
```

Every multiple of 8 in one bucket. A map keyed on addresses, IDs, offsets, or timestamps in
even units degenerates into a single chain, and rehashing does not help because doubling
preserves the property.

**libc++ pays for the slower modulo deliberately, to avoid exactly this.** Its growth
sequence is prime — measured: 2, 5, 11, 23, 47, 97, 197, 397 — so the whole hash affects the
index. It is slower on my benchmark's string keys and immune to a failure case I still have.

That is what a trade-off looks like once it is measured rather than argued: both statements
are true at once, and neither container is simply better.

---

## 6. Vector: parity, which is the expected result

`push_back` of `int` lands at parity with `std::vector` — range 0.85–1.28 across runs. That
is not a moral victory; it is what should happen. At `-O3` both compile down to roughly the
same bounds check, store and increment. It says the growth strategy is right and nothing
stupid is happening per element.

---

## 7. A methodology warning worth more than any number here

The benchmark must be built under `release`. The repo's other two presets pass no `-O` flag,
and one also carries sanitizer instrumentation.

Run this same benchmark under the `dev` preset and it reports:

| Operation | dev preset | release | |
|---|---|---|---|
| `vector` push_back | **0.33x**, range 0.32–0.35, "mine wins" | 1.08x, parity | **reversed** |
| `hashmap` insert | 2.43x | 5.70x | loss more than halved |

Under `dev` the measurement says my vector is three times faster than `std::vector`, with a
*tight* range around that answer. It is flatly false. At `-O0` nothing inlines, so a deep
stack of thin abstractions — `std::vector`'s iterator wrappers, allocator indirection,
`__builtin` calls — pays a real function call per layer, while a three-member class with a
raw loop pays almost nothing. An unoptimized benchmark measures **abstraction depth**, not
speed, and it systematically flatters whichever implementation is more primitive.

Note that it did not merely understate the result. It *reversed* it, and it also hid more
than half the insert loss. An unoptimized benchmark is not a pessimistic benchmark; it is an
unrelated one — and, as the tight range above shows, it can be confidently and reproducibly
unrelated. **Precision is not accuracy.**

---

## 8. Summary

| | |
|---|---|
| Insert | 5.7x slower. 47% rehashing, the rest ~4 key copies and 390 unrequested constructions per 100 inserts, all from `new T[n]`. |
| Lookup, misses | ~1.7x faster. Lower load-factor ceiling, cheaper modulo, and a miss walks the whole chain. |
| Lookup, hits | Parity. No claim. |
| `vector` push_back | Parity, as expected at `-O3`. |
| Known weakness | Power-of-two bucket count is vulnerable to structured integer keys; libc++ uses primes and pays a slower modulo to avoid it. |
| Deferred | Raw storage + placement `new`. Named, priced at the 5.7x above, and out of scope for this submission. |
