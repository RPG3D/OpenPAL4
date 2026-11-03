#include "pal4/heap_zero_allocator.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IHeapZeroRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size) override {
        requested_size = size;
        ++allocation_calls;
        return next_result;
    }

    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Zero(const std::uint32_t address,
              const std::uint32_t size) override {
        zero_address = address;
        zero_size = size;
        ++zero_calls;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_result = 0x1008u;
    std::uint32_t requested_size = 0u;
    std::uint32_t zero_address = 0u;
    std::uint32_t zero_size = 0u;
    int allocation_calls = 0;
    int zero_calls = 0;
};

void CheckOverflowAndAllocationFailure() {
    Memory memory;
    memory.next_result = 0u;
    Check(pal4::HeapZeroAllocator::AllocateAndZeroWrapper(
              0x40000000u, 8u, 0xdeadbeefu, memory) == 0u,
          "failed allocation returns zero");
    Check(memory.requested_size == 0u && memory.zero_calls == 0,
          "element multiplication wraps at 32 bits and failure skips zeroing");
}

void CheckOrdinaryBlockLengths() {
    constexpr std::array<std::uint32_t, 5> sizes{
        16u, 24u, 32u, 40u, 48u};
    for (const std::uint32_t size : sizes) {
        Memory memory;
        memory.words[0x1004u] = size | 1u;
        Check(pal4::HeapZeroAllocator::AllocateAndZero(3u, 7u, memory) ==
                  0x1008u,
              "ordinary allocation returns payload");
        Check(memory.requested_size == 21u && memory.zero_calls == 1 &&
                  memory.zero_address == 0x1008u &&
                  memory.zero_size == size - 4u,
              "ordinary block clears observed capacity minus four");
    }
}

void CheckMappedBlockSkipsZeroing() {
    Memory memory;
    memory.words[0x1004u] = 0x1002u;
    Check(pal4::HeapZeroAllocator::AllocateAndZero(2u, 100u, memory) ==
              0x1008u,
          "mapped allocation still returns the payload");
    Check(memory.zero_calls == 0,
          "mapping flag bypasses all observed zero writes");
}

}  // namespace

int main() {
    CheckOverflowAndAllocationFailure();
    CheckOrdinaryBlockLengths();
    CheckMappedBlockSkipsZeroing();
    std::cout << "3 heap zero allocator test cases passed\n";
}
