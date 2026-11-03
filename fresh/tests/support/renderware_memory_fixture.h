#pragma once
#include <rwcore.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

// Shared diagnostic allocator: exact ownership tracking, fail-on-Nth callback,
// and forced realloc movement. It is not the original game allocator.
namespace pal4::test::memory_fixture {
inline std::unordered_map<void *, size_t> live;
inline bool invalidFree{};
inline unsigned failNth{}, reallocations{};
inline bool Fail() { return failNth && --failNth == 0; }
inline void *Allocate(size_t n, RwUInt32) {
    if (Fail())
        return nullptr;
    auto *p = std::malloc(n);
    if (p)
        live.emplace(p, n);
    return p;
}
inline void Free(void *p) {
    if (p && live.erase(p) != 1)
        invalidFree = true;
    std::free(p);
}
inline void *Reallocate(void *p, size_t n, RwUInt32) {
    ++reallocations;
    if (Fail())
        return nullptr;
    // Force movement to verify the SDK repairs every free-entry backpointer.
    auto *q = std::malloc(n);
    if (!q)
        return nullptr;
    if (p)
        std::memcpy(q, p, (std::min)(n, live.at(p)));
    Free(p);
    live.emplace(q, n);
    return q;
}
inline void *Callocate(size_t n, size_t size, RwUInt32 hint) {
    auto *p = Allocate(n * size, hint);
    if (p)
        std::memset(p, 0, n * size);
    return p;
}
} // namespace pal4::test::memory_fixture
