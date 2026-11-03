#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include "support/renderware_memory_fixture.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

// Audited SDK exports used only to probe the resource allocator's raw boundary.
// Production resource users call RwResourcesAllocateResEntry/FreeResEntry.
extern "C" {
RwBool _rwResHeapInit(void *, RwUInt32);
void *_rwResHeapAlloc(void *, RwUInt32);
void _rwResHeapFree(void *);
}
namespace {
void Check(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
using namespace pal4::test::memory_fixture;
struct Session {
    HWND window{};
    Session() {
        const RwMemoryFunctions memory{Allocate, Free, Reallocate, Callocate};
        Check(RwEngineInit(&memory, 0, 4 * 1024 * 1024), "engine init");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 heap", WS_OVERLAPPEDWINDOW, 64, 64, nullptr,
                               nullptr, GetModuleHandleA(nullptr), nullptr);
        Check(window, "window");
        RwEngineOpenParams params{window};
        Check(RwEngineOpen(&params) && RwEngineStart(), "engine start");
    }
    ~Session() {
        failNth = 0;
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
        DestroyWindow(window);
    }
};
struct DropHeap {
    void operator()(RxHeap *h) const { RxHeapDestroy(h); }
};
using Heap = std::unique_ptr<RxHeap, DropHeap>;
Heap Create(unsigned size = 1024) {
    Heap h(RxHeapCreate(size));
    Check(h != nullptr, "heap create");
    return h;
}
rxHeapBlockHeader *Header(void *p) { return static_cast<rxHeapBlockHeader *>(p) - 1; }
void CheckFreeTable(RxHeap *h) {
    Check(h->entriesUsed <= h->entriesAlloced, "free table bounds");
    for (unsigned i = 0; i < h->entriesUsed; ++i) {
        auto &e = h->freeBlocks[i];
        Check(e.ptr && e.ptr->freeEntry == &e && e.ptr->size == e.size, "reciprocal free entry");
    }
}
struct ResourceHeader {
    void *heap;
    ResourceHeader *next, *prev;
    unsigned size, flags;
    unsigned pad[3];
};
static_assert(sizeof(ResourceHeader) == 32 && sizeof(rxHeapBlockHeader) == 32 && sizeof(RxHeap) == 28);
static_assert(sizeof(rxHeapFreeBlock) == 8 && sizeof(rxHeapSuperBlockDescriptor) == 12);
static_assert(offsetof(RwGlobals, stringFuncs) == 196 && offsetof(RwGlobals, memoryFuncs) == 264);
void ResourceLayout() {
    alignas(32) std::array<unsigned char, 640> memory{};
    Check(!_rwResHeapInit(memory.data(), 95), "resource arena needs payload after two aligned headers");
    Check(_rwResHeapInit(memory.data(), 96), "minimum aligned arena");
    auto *p = _rwResHeapAlloc(memory.data(), 1);
    Check(p == memory.data() + 64 && !_rwResHeapAlloc(memory.data(), 1),
          "minimum32-byte payload exhausted");
    _rwResHeapFree(p);
    for (int offset = 0; offset < 32; ++offset) {
        memory.fill(0xa7);
        auto *arena = memory.data() + offset;
        Check(_rwResHeapInit(arena, 512), "unaligned caller arena");
        p = _rwResHeapAlloc(arena, 33);
        Check(p && (reinterpret_cast<std::uintptr_t>(p) & 31) == 0, "resource32-byte alignment");
        auto *header = static_cast<ResourceHeader *>(p) - 1;
        Check(header->size == 64 && header->flags == 1 && header->heap == arena,
              "rounded resource block");
        _rwResHeapFree(p);
        for (int i = 0; i < offset; ++i)
            Check(memory[i] == 0xa7, "arena prefix guard");
        for (int i = offset + 512; i < 640; ++i)
            Check(memory[i] == 0xa7, "arena suffix guard");
    }
    for (unsigned request : {352u, 384u, 416u}) {
        Check(_rwResHeapInit(memory.data(), 512), "split threshold arena");
        p = _rwResHeapAlloc(memory.data(), request);
        // Initial payload448. A64-byte remainder is swallowed by strict >64 split.
        Check(p && (static_cast<ResourceHeader *>(p) - 1)->size == (request == 352 ? 352u : 448u),
              "resource allocator splits only above64 bytes of remainder");
        _rwResHeapFree(p);
    }
    Check(_rwResHeapInit(memory.data(), 512), "zero-size arena");
    p = _rwResHeapAlloc(memory.data(), 0);
    Check(p && (static_cast<ResourceHeader *>(p) - 1)->size == 0, "zero request consumes a header");
    _rwResHeapFree(p);
}
void ResourceMerge() {
    alignas(32) std::array<unsigned char, 1024> memory{};
    Check(_rwResHeapInit(memory.data(), memory.size()), "resource merge arena");
    auto *a = _rwResHeapAlloc(memory.data(), 64);
    auto *b = _rwResHeapAlloc(memory.data(), 64);
    auto *c = _rwResHeapAlloc(memory.data(), 64);
    Check(a && b && c, "three resource blocks");
    std::memset(b, 0x5a, 64);
    _rwResHeapFree(a);
    _rwResHeapFree(c);
    for (int i = 0; i < 64; ++i)
        Check(static_cast<unsigned char *>(b)[i] == 0x5a, "live neighbor preserved");
    _rwResHeapFree(b);
    auto *large = _rwResHeapAlloc(memory.data(), 960);
    Check(large == a && !_rwResHeapAlloc(memory.data(), 32),
          "both-neighbor coalescing restores entire arena");
    _rwResHeapFree(large);
}
void Layout() {
    for (unsigned requested : {1u, 1025u}) {
        auto h = Create(requested);
        Check(h->superBlockSize == (requested == 1 ? 1024u : 1056u),
              "minimum and32-byte superblock rounding");
        Check((reinterpret_cast<std::uintptr_t>(h->head->start) & 127) == 0,
              "superblock start128-byte alignment");
        Check(!h->dirty && h->entriesAlloced == 32 && h->entriesUsed == 1,
              "initial free table and clean state");
        Check(h->freeBlocks[0].size == h->superBlockSize - 96, "two sentinels plus payload header");
        auto *p = RxHeapAlloc(h.get(), 33);
        Check(p && (reinterpret_cast<std::uintptr_t>(p) & 31) == 0 && Header(p)->size == 64 && h->dirty,
              "public allocation alignment,rounding,dirty");
        CheckFreeTable(h.get());
        RxHeapFree(h.get(), p);
        CheckFreeTable(h.get());
    }
    RxHeapDestroy(nullptr);
}
void SplitMerge() {
    for (unsigned request : {672u, 704u}) {
        auto h = Create();
        auto *p = RxHeapAlloc(h.get(), request);
        Check(p && Header(p)->size == (request == 672 ? 672u : 928u), "Rx split threshold256");
        Check(h->entriesUsed == (request == 672 ? 1u : 0u), "whole-block removal from free table");
        RxHeapFree(h.get(), p);
        CheckFreeTable(h.get());
    }
    auto h = Create();
    void *p[4];
    for (auto &block : p) {
        block = RxHeapAlloc(h.get(), 64);
        Check(block, "merge allocation");
    }
    RxHeapFree(h.get(), p[1]);
    Check(h->entriesUsed == 2, "isolated free creates entry");
    RxHeapFree(h.get(), p[2]);
    Check(Header(p[1])->size == 160, "previous free merge");
    RxHeapFree(h.get(), p[3]);
    Check(h->entriesUsed == 1, "both-neighbor merge removes duplicate entry");
    RxHeapFree(h.get(), p[0]);
    Check(h->entriesUsed == 1 && h->freeBlocks[0].size == 928, "next merge restores payload");
    CheckFreeTable(h.get());
}
void FreeList() {
    auto h = Create(32768);
    std::array<void *, 70> p{};
    for (auto &block : p) {
        block = RxHeapAlloc(h.get(), 64);
        Check(block, "fragment allocation");
    }
    auto *old = h->freeBlocks;
    const auto calls = reallocations;
    for (int i = 1; i < 68; i += 2) {
        RxHeapFree(h.get(), p[i]);
        CheckFreeTable(h.get());
    }
    Check(h->entriesAlloced == 64 && h->entriesUsed == 35 && h->freeBlocks != old &&
              reallocations == calls + 1,
          "free table grows32 entries and repairs moved pointers");
    // Force removal of a non-final entry; the packed table must repair the moved last entry.
    auto *a = RxHeapAlloc(h.get(), 64);
    Check(a, "free table reuse");
    CheckFreeTable(h.get());
    Check(RxHeapReset(h.get()) && h->entriesUsed == 1 && !h->dirty, "reset fragmented heap");
}
void Realloc() {
    auto h = Create(8192);
    auto *a = RxHeapAlloc(h.get(), 64);
    auto *b = RxHeapAlloc(h.get(), 512);
    auto *c = RxHeapAlloc(h.get(), 64);
    Check(a && b && c, "realloc blocks");
    std::memset(a, 0x39, 64);
    RxHeapFree(h.get(), b);
    Check(RxHeapRealloc(h.get(), a, 160, FALSE) == a && Header(a)->size == 160,
          "in-place expansion leaves split tail");
    Check(RxHeapRealloc(h.get(), a, 448, FALSE) == a && Header(a)->size == 608,
          "in-place expansion consumes remainder below256");
    Check(RxHeapRealloc(h.get(), a, 512, FALSE) == a && Header(a)->size == 608,
          "small shrink retains physical block");
    Check(RxHeapRealloc(h.get(), a, 256, FALSE) == a && Header(a)->size == 256,
          "large shrink adds free block");
    CheckFreeTable(h.get());
    auto *moved = RxHeapRealloc(h.get(), a, 4096, FALSE);
    Check(moved && moved != a, "Release ignores allowCopy false and moves allocation");
    for (int i = 0; i < 64; ++i)
        Check(static_cast<unsigned char *>(moved)[i] == 0x39, "moved payload retained");
    CheckFreeTable(h.get());
    failNth = 1;
    Check(!RxHeapRealloc(h.get(), moved, 65536, FALSE) && !failNth,
          "failed replacement allocation leaves original block");
    Check(static_cast<unsigned char *>(moved)[0] == 0x39 && Header(moved)->freeEntry == nullptr,
          "failure preserves live original");
    RxHeapFree(h.get(), moved);
    RxHeapFree(h.get(), c);
    CheckFreeTable(h.get());
}
void Reset() {
    auto h = Create();
    const auto before = live.size();
    Check(RxHeapAlloc(h.get(), 5000) && RxHeapAlloc(h.get(), 7000), "grow two superblocks");
    Check(live.size() == before + 2 && h->dirty, "growth retains original arena");
    const auto *head = h->head;
    Check(RxHeapReset(h.get()) && h->head == head && h->entriesUsed == 3 && !h->dirty &&
              live.size() == before + 2,
          "reset reclaims blocks without freeing superblock allocations");
    CheckFreeTable(h.get());
    auto *table = h->freeBlocks;
    Check(RxHeapReset(h.get()) && h->freeBlocks == table, "clean macro reset short-circuits");
}
void Failures() {
    const auto baseline = live;
    for (unsigned n : {1u, 2u, 3u}) {
        failNth = n;
        Check(!RxHeapCreate(1024) && !failNth && live == baseline,
              "create failure rolls back heap/superblock/free-table allocations");
    }
    auto h = Create(65536);
    std::array<void *, 70> p{};
    for (auto &block : p) {
        block = RxHeapAlloc(h.get(), 512);
        Check(block, "failure fragmentation");
    }
    for (int i = 1; i < 62; i += 2)
        RxHeapFree(h.get(), p[i]);
    Check(h->entriesUsed == 32 && h->entriesAlloced == 32, "full free table");
    failNth = 1;
    Check(RxHeapRealloc(h.get(), p[0], 128, FALSE) == p[0] && !failNth && Header(p[0])->size == 512,
          "failed shrink metadata preserves large block and still returns original pointer");
    failNth = 1;
    RxHeapFree(h.get(), p[63]);
    Check(!failNth && h->entriesUsed == 32 && h->entriesAlloced == 32 &&
              Header(p[63])->freeEntry == nullptr,
          "void Free cannot report failed metadata growth and leaves block allocated");
    CheckFreeTable(h.get());
    Check(RxHeapReset(h.get()) && h->entriesUsed == 1, "reset reclaims lost-to-Free block");
}
void ResetFailure() {
    auto h = Create();
    for (int i = 0; i < 33; ++i)
        Check(RxHeapAlloc(h.get(), 2048), "whole-superblock growth before reset");
    Check(h->entriesAlloced == 32 && h->entriesUsed == 1, "whole allocations keep free table small");
    failNth = 1;
    Check(!RxHeapReset(h.get()) && !failNth && h->dirty && h->entriesUsed == 32,
          "reset can fail after rebuilding only32 superblocks");
    Check(RxHeapReset(h.get()) && !h->dirty && h->entriesUsed == 34 && h->entriesAlloced == 64,
          "retry reset rebuilds all superblocks after metadata growth succeeds");
    CheckFreeTable(h.get());
}
DWORD GuardedAllocate(RxHeap *heap) {
    __try {
        (void)RxHeapAlloc(heap, 32);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
void EmptyFreeList() {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    const auto page = info.dwPageSize;
    auto *area = static_cast<unsigned char *>(
        VirtualAlloc(nullptr, 2 * page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(area, "guard allocation");
    DWORD old{};
    Check(VirtualProtect(area, page, PAGE_NOACCESS, &old), "guard page");
    RxHeap heap{};
    heap.freeBlocks = reinterpret_cast<rxHeapFreeBlock *>(area + page);
    const auto code = GuardedAllocate(&heap);
    VirtualFree(area, 0, MEM_RELEASE);
    Check(code == EXCEPTION_ACCESS_VIOLATION,
          "zero free entries still read freeBlocks[-1] before any growth attempt");
}
} // namespace
int main(int argc, char **argv) {
    try {
        Check(argc == 2, "one case");
        {
            Session session;
            const auto baseline = live;
            std::string_view name = argv[1];
            if (name == "resource_layout")
                ResourceLayout();
            else if (name == "resource_merge")
                ResourceMerge();
            else if (name == "layout")
                Layout();
            else if (name == "split_merge")
                SplitMerge();
            else if (name == "freelist")
                FreeList();
            else if (name == "realloc")
                Realloc();
            else if (name == "reset")
                Reset();
            else if (name == "failures")
                Failures();
            else if (name == "empty_free_list")
                EmptyFreeList();
            else if (name == "reset_failure")
                ResetFailure();
            else
                throw std::runtime_error("unknown heap case");
            Check(live == baseline && !invalidFree, "fixture allocations released exactly once");
        }
        Check(live.empty() && !invalidFree, "engine teardown allocations");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
