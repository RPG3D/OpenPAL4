#include "pal4/heap_allocator.h"
#include "pal4/heap_size_class.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {

constexpr std::uint32_t kHeap = pal4::HeapAllocator::kHeapAddress;
constexpr std::uint32_t kUnsorted = kHeap + 0x34u;
constexpr std::uint32_t kTop = kHeap + 0x2cu;
constexpr std::uint32_t kAllocated = kHeap + 0x394u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IHeapRuntime {
public:
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    void ManageBlocks(const std::uint32_t address) override {
        Check(address == kHeap, "manage receives the heap base");
        ++manage_calls;
        if (initialize_on_manage) {
            Write(kHeap, 1u);
            Write(kTop, 0x7000u);
            Write(0x7004u, 0x100u);
        }
    }

    [[nodiscard]] std::uint32_t Extend(const std::uint32_t size,
                                       const std::uint32_t address) override {
        Check(address == kHeap, "extend receives the heap base");
        last_extended_size = size;
        ++extend_calls;
        return extended_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    int manage_calls = 0;
    int extend_calls = 0;
    bool initialize_on_manage = false;
    std::uint32_t last_extended_size = 0;
    std::uint32_t extended_result = 0x9000u;
};

std::uint32_t Bin(const std::uint32_t index) {
    return kTop + index * 8u;
}

void SeedEmptyBin(Memory& memory, const std::uint32_t index) {
    const std::uint32_t sentinel = Bin(index);
    memory.Write(sentinel + 8u, sentinel);
    memory.Write(sentinel + 12u, sentinel);
}

void SeedUnsortedEmpty(Memory& memory) {
    memory.Write(kHeap + 0x40u, kUnsorted);
}

void CheckRejectedAndInitializedTop() {
    Memory memory;
    Check(pal4::HeapAllocator::MemAlloc(0xffffffe0u, memory) == 0u,
          "overflowing request is rejected before heap access");
    Check(memory.manage_calls == 0, "rejection has no side effects");
    memory.initialize_on_manage = true;
    Check(pal4::HeapAllocator::HMalloc(0u, 0xdeadbeefu, memory) == 0x7008u,
          "uninitialized heap is managed, then top is split");
    Check(memory.manage_calls == 1, "initialization is called once");
    Check(memory.Read(kTop) == 0x7010u, "top advances by minimum chunk size");
    Check(memory.Read(0x7004u) == 0x11u, "allocated header is written");
    Check(memory.Read(0x7014u) == 0xf1u, "top remainder header is written");
    Check(memory.Read(kAllocated) == 12u, "payload accounting follows IDA helper");
}

void CheckTopExtension() {
    Memory memory;
    memory.Write(kHeap, 1u);
    memory.Write(kTop, 0x7000u);
    memory.Write(0x7004u, 0x20u);
    SeedEmptyBin(memory, 5u);
    SeedUnsortedEmpty(memory);
    Check(pal4::HeapAllocator::Allocate(32u, memory) == 0x9000u,
          "undersized top delegates to extension");
    Check(memory.last_extended_size == 40u && memory.extend_calls == 1,
          "extension receives normalized size");
    Check(memory.Read(kAllocated) == 0u,
          "extension return does not pass local accounting tail");
}

void CheckFastBin() {
    Memory memory;
    memory.Write(kHeap, 0x11u);
    memory.Write(kHeap + 4u, 0x1000u);
    memory.Write(0x1004u, 0x11u);
    memory.Write(0x1008u, 0x1100u);
    memory.Write(0x1014u, 1u);
    Check(pal4::HeapAllocator::Allocate(4u, memory) == 0x1008u,
          "fast bin returns its first chunk");
    Check(memory.Read(kHeap + 4u) == 0x1100u, "fast bin head advances");
    Check(memory.Read(kAllocated) == 12u, "fast-bin payload is counted");
}

void CheckSmallExactBin() {
    Memory memory;
    memory.Write(kHeap, 1u);
    const std::uint32_t sentinel = Bin(3u);
    memory.Write(sentinel + 8u, 0x2000u);
    memory.Write(sentinel + 12u, 0x2000u);
    memory.Write(0x2004u, 0x19u);
    memory.Write(0x2008u, sentinel);
    memory.Write(0x200cu, sentinel);
    Check(pal4::HeapAllocator::Allocate(17u, memory) == 0x2008u,
          "small exact bin returns its chunk");
    Check(memory.Read(sentinel + 8u) == sentinel &&
              memory.Read(sentinel + 12u) == sentinel,
          "small exact bin is unlinked");
    Check((memory.Read(0x201cu) & 1u) != 0u,
          "following chunk gets previous-in-use bit");
    Check(memory.Read(kAllocated) == 20u, "small payload is counted");
}

void CheckUnsortedExactAndRemainder() {
    Memory exact;
    exact.Write(kHeap, 1u);
    SeedEmptyBin(exact, 3u);
    exact.Write(kHeap + 0x40u, 0x3000u);
    exact.Write(0x3004u, 0x19u);
    exact.Write(0x300cu, kUnsorted);
    Check(pal4::HeapAllocator::Allocate(17u, exact) == 0x3008u,
          "unsorted exact chunk is returned");
    Check(exact.Read(kHeap + 0x40u) == kUnsorted,
          "unsorted head is cleared");

    Memory split;
    split.Write(kHeap, 1u);
    SeedEmptyBin(split, 3u);
    split.Write(kHeap + 0x30u, 0x4000u);
    split.Write(kHeap + 0x40u, 0x4000u);
    split.Write(0x4004u, 0x51u);
    split.Write(0x400cu, kUnsorted);
    Check(pal4::HeapAllocator::Allocate(17u, split) == 0x4008u,
          "single unsorted remainder is split");
    Check(split.Read(kHeap + 0x30u) == 0x4018u,
          "new remainder becomes last remainder");
    Check(split.Read(0x401cu) == 0x39u &&
              split.Read(0x4050u) == 56u,
          "remainder header and footer are written");
    Check(split.Read(kAllocated) == 20u, "split payload is counted");
}

void CheckLargeBinAndBitmap() {
    Memory large;
    large.Write(kHeap, 1u);
    SeedUnsortedEmpty(large);
    const std::uint32_t large_bin = Bin(pal4::HeapSizeClass::Index(256u));
    large.Write(large_bin + 8u, 0x5000u);
    large.Write(large_bin + 12u, 0x5000u);
    large.Write(0x5004u, 0x101u);
    large.Write(0x5008u, large_bin);
    large.Write(0x500cu, large_bin);
    Check(pal4::HeapAllocator::Allocate(245u, large) == 0x5008u,
          "large exact bin returns a fitting chunk");
    Check(large.Read(kAllocated) == 252u, "large payload is counted");

    Memory bitmap;
    bitmap.Write(kHeap, 1u);
    SeedUnsortedEmpty(bitmap);
    SeedEmptyBin(bitmap, 3u);
    const std::uint32_t next_bin = Bin(4u);
    bitmap.Write(next_bin + 8u, 0x6000u);
    bitmap.Write(next_bin + 12u, 0x6000u);
    bitmap.Write(0x6004u, 0x21u);
    bitmap.Write(0x6008u, next_bin);
    bitmap.Write(0x600cu, next_bin);
    bitmap.Write(kHeap + 0x334u, 1u << 4);
    Check(pal4::HeapAllocator::Allocate(17u, bitmap) == 0x6008u,
          "bitmap search selects the next occupied bin");
    Check(bitmap.Read(next_bin + 12u) == next_bin,
          "bitmap-selected chunk is unlinked");
}

void CheckUnsortedRebinAndLargeSplit() {
    Memory rebin;
    rebin.Write(kHeap, 1u);
    SeedEmptyBin(rebin, 3u);
    SeedEmptyBin(rebin, 4u);
    rebin.Write(kHeap + 0x40u, 0x6800u);
    rebin.Write(0x6804u, 0x21u);
    rebin.Write(0x680cu, kUnsorted);
    Check(pal4::HeapAllocator::Allocate(17u, rebin) == 0x6808u,
          "unsorted block is rebinned then selected by bitmap");
    Check((rebin.Read(kHeap + 0x334u) & (1u << 4)) != 0u,
          "rebinned size class is recorded in bitmap");

    Memory split;
    split.Write(kHeap, 1u);
    SeedUnsortedEmpty(split);
    SeedEmptyBin(split, pal4::HeapSizeClass::Index(256u));
    const std::uint32_t larger_index = pal4::HeapSizeClass::Index(320u);
    const std::uint32_t sentinel = Bin(larger_index);
    split.Write(sentinel + 8u, 0x9000u);
    split.Write(sentinel + 12u, 0x9000u);
    split.Write(0x9004u, 0x141u);
    split.Write(0x9008u, sentinel);
    split.Write(0x900cu, sentinel);
    split.Write(kHeap + 0x334u + (larger_index >> 5) * 4u,
                1u << (larger_index & 31u));
    Check(pal4::HeapAllocator::Allocate(245u, split) == 0x9008u,
          "larger bin is split for a large request");
    Check(split.Read(0x9104u) == 0x41u &&
              split.Read(0x9140u) == 64u,
          "large split writes remainder header and footer");
    Check(split.Read(kAllocated) == 252u,
          "large split accounts only for allocated payload");
}

void CheckPayloadSizeFlags() {
    Memory memory;
    Check(pal4::HeapAllocator::PayloadSize(0u, memory) == 0u,
          "null payload has zero size");
    memory.Write(0x8004u, 0x42u);
    Check(pal4::HeapAllocator::PayloadSize(0x8008u, memory) == 56u,
          "mapped-block flag subtracts eight bytes");
    memory.Write(0x8004u, 0x41u);
    Check(pal4::HeapAllocator::PayloadSize(0x8008u, memory) == 0u,
          "missing following previous-in-use bit returns zero");
}

}  // namespace

int main() {
    CheckRejectedAndInitializedTop();
    CheckTopExtension();
    CheckFastBin();
    CheckSmallExactBin();
    CheckUnsortedExactAndRemainder();
    CheckLargeBinAndBitmap();
    CheckUnsortedRebinAndLargeSplit();
    CheckPayloadSizeFlags();
    std::cout << "8 heap allocator test cases passed\n";
}
