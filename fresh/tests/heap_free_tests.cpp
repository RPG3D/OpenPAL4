#include "pal4/heap_free.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {

constexpr std::uint32_t kHeap = pal4::HeapAllocator::kHeapAddress;
constexpr std::uint32_t kUnsorted = kHeap + 0x34u;
constexpr std::uint32_t kAllocated = kHeap + 0x394u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IHeapFreeRuntime {
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
        Check(address == kHeap, "manage receives heap base");
        ++manage_calls;
    }

    [[nodiscard]] std::uint32_t Extend(std::uint32_t,
                                       std::uint32_t) override {
        Check(false, "free must not extend heap");
        return 0u;
    }

    [[nodiscard]] std::uint32_t AdjustHeapSize(
        const std::uint32_t amount,
        const std::uint32_t address) override {
        Check(address == kHeap, "trim receives heap base");
        trimmed_amount = amount;
        ++trim_calls;
        return trim_result;
    }

    [[nodiscard]] std::uint32_t ReleaseVirtualMemory(
        const std::uint32_t base,
        const std::uint32_t size) override {
        released_base = base;
        released_size = size;
        ++release_calls;
        return release_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t released_base = 0;
    std::uint32_t released_size = 0;
    std::uint32_t trimmed_amount = 0;
    std::uint32_t release_result = 73u;
    std::uint32_t trim_result = 37u;
    int manage_calls = 0;
    int release_calls = 0;
    int trim_calls = 0;
};

void SeedUnsorted(Memory& memory) {
    memory.Write(kHeap + 0x3cu, kUnsorted);
    memory.Write(kHeap + 0x40u, kUnsorted);
}

void CheckNullAndFastBin() {
    Memory memory;
    memory.Write(kAllocated, 100u);
    Check(pal4::HeapFree::MemFree(0u, memory) == 0u,
          "null free returns zero");
    Check(memory.Read(kAllocated) == 100u,
          "null free subtracts zero payload bytes");

    memory.Write(kHeap, 0x11u);
    memory.Write(0x1004u, 0x11u);
    memory.Write(0x1014u, 1u);
    memory.Write(kHeap + 4u, 0x1100u);
    Check(pal4::HeapFree::Free(0x1008u, memory) == 0x1000u,
          "small free returns chunk header address");
    Check(memory.Read(kHeap) == 0x13u &&
              memory.Read(kHeap + 4u) == 0x1000u &&
              memory.Read(0x1008u) == 0x1100u,
          "fast-bin flags and next pointer are updated");
    Check(memory.Read(kAllocated) == 88u,
          "fast-bin payload bytes are subtracted");
}

void CheckMappedRelease() {
    Memory memory;
    memory.Write(kHeap, 1u);
    memory.Write(0x2000u, 5u);
    memory.Write(0x2004u, 0x1002u);
    memory.Write(kHeap + 0x350u, 3u);
    memory.Write(kHeap + 0x364u, 0x3000u);
    memory.Write(kAllocated, 5000u);
    Check(pal4::HeapFree::Free(0x2008u, memory) == 73u,
          "mapped free propagates platform release result");
    Check(memory.released_base == 0x1ffbu &&
              memory.released_size == 0x1005u &&
              memory.release_calls == 1,
          "mapping base and span include leading padding");
    Check(memory.Read(kHeap + 0x350u) == 2u &&
              memory.Read(kHeap + 0x364u) == 0x1ffbu,
          "mapping counters are decremented");
    Check(memory.Read(kAllocated) == 5000u - 0xff8u,
          "mapped payload accounting is reversed");
}

void CheckTopAndBackwardCoalescing() {
    Memory top;
    top.Write(kHeap, 1u);
    top.Write(kHeap + 0x2cu, 0x3040u);
    top.Write(0x3004u, 0x41u);
    top.Write(0x3044u, 0x81u);
    top.Write(kAllocated, 100u);
    Check(pal4::HeapFree::Free(0x3008u, top) == 0x3000u,
          "freed block adjacent to top becomes the new top");
    Check(top.Read(kHeap + 0x2cu) == 0x3000u &&
              top.Read(0x3004u) == 0xc1u,
          "top coalesces both block sizes");
    Check(top.Read(kAllocated) == 40u,
          "ordinary payload size is subtracted");

    Memory backward;
    backward.Write(kHeap, 1u);
    backward.Write(kHeap + 0x2cu, 0x4040u);
    backward.Write(0x3fe8u, kUnsorted);
    backward.Write(0x3fecu, kUnsorted);
    backward.Write(kUnsorted + 8u, 0x3fe0u);
    backward.Write(kUnsorted + 12u, 0x3fe0u);
    backward.Write(0x4000u, 0x20u);
    backward.Write(0x4004u, 0x40u);
    backward.Write(0x4044u, 0x81u);
    Check(pal4::HeapFree::Free(0x4008u, backward) == 0x3fe0u,
          "free coalesces previous block before top");
    Check(backward.Read(kHeap + 0x2cu) == 0x3fe0u &&
              backward.Read(0x3fe4u) == 0xe1u,
          "backward and top spans are combined");
    Check(backward.Read(kUnsorted + 8u) == kUnsorted &&
              backward.Read(kUnsorted + 12u) == kUnsorted,
          "previous free block is unlinked");
}

void CheckUnsortedAndForwardCoalescing() {
    Memory memory;
    memory.Write(kHeap, 1u);
    memory.Write(kHeap + 0x2cu, 0x8000u);
    SeedUnsorted(memory);
    memory.Write(0x5004u, 0x41u);
    memory.Write(0x5044u, 0x21u);
    memory.Write(0x5064u, 0u);
    const std::uint32_t sentinel = 0x6000u;
    memory.Write(0x5048u, sentinel);
    memory.Write(0x504cu, sentinel);
    memory.Write(sentinel + 8u, 0x5040u);
    memory.Write(sentinel + 12u, 0x5040u);
    Check(pal4::HeapFree::Free(0x5008u, memory) == 0x5000u,
          "free coalesces a following free block");
    Check(memory.Read(0x5004u) == 0x61u &&
              memory.Read(0x5060u) == 96u,
          "combined unsorted block gets header and footer");
    Check(memory.Read(kHeap + 0x3cu) == 0x5000u &&
              memory.Read(kUnsorted + 12u) == 0x5000u,
          "combined block is inserted into unsorted chain");
    Check(memory.Read(sentinel + 8u) == sentinel &&
              memory.Read(sentinel + 12u) == sentinel,
          "following free block is unlinked");
}

void CheckLargeTrim() {
    Memory memory;
    memory.Write(kHeap, 1u);
    memory.Write(kHeap + 0x2cu, 0x27000u);
    memory.Write(0x7004u, 0x20001u);
    memory.Write(0x27004u, 0x20001u);
    memory.Write(kHeap + 0x344u, 0x10000u);
    memory.Write(kHeap + 0x348u, 0x1000u);
    Check(pal4::HeapFree::Free(0x7008u, memory) == 37u,
          "large coalesced top invokes heap trimming");
    Check(memory.trim_calls == 1 && memory.trimmed_amount == 0x1000u,
          "trim receives configured amount");
}

}  // namespace

int main() {
    CheckNullAndFastBin();
    CheckMappedRelease();
    CheckTopAndBackwardCoalescing();
    CheckUnsortedAndForwardCoalescing();
    CheckLargeTrim();
    std::cout << "5 heap free test cases passed\n";
}
