#include "pal4/heap_allocator.h"
#include "pal4/heap_extender.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <unordered_map>

namespace {

constexpr std::uint32_t kHeap = 0x8dd4a0u;
constexpr std::uint32_t kFailure = 0xffffffffu;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory : public pal4::IHeapExtensionRuntime {
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
        Check(address == kHeap, "manage uses the supplied heap");
        ++manage_calls;
    }

    [[nodiscard]] std::uint32_t RetryAllocate(
        const std::uint32_t request) override {
        retry_request = request;
        return retry_result;
    }

    [[nodiscard]] std::uint32_t AllocateVirtualMemory(
        const std::uint32_t size) override {
        virtual_size = size;
        ++virtual_calls;
        if (virtual_results.empty()) {
            return kFailure;
        }
        const std::uint32_t result = virtual_results.front();
        virtual_results.pop_front();
        return result;
    }

    [[nodiscard]] std::uint32_t ExtendProgramBreak(
        const std::uint32_t size) override {
        last_break_size = size;
        ++break_calls;
        if (break_results.empty()) {
            return kFailure;
        }
        const std::uint32_t result = break_results.front();
        break_results.pop_front();
        return result;
    }

    void FreeBlock(const std::uint32_t payload) override {
        ++free_calls;
        freed_payload = payload;
        free_observed_guard = Read(kHeap + 836u);
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::deque<std::uint32_t> virtual_results;
    std::deque<std::uint32_t> break_results;
    std::uint32_t retry_request = 0;
    std::uint32_t retry_result = 0x1234u;
    std::uint32_t virtual_size = 0;
    std::uint32_t last_break_size = 0;
    std::uint32_t freed_payload = 0;
    std::uint32_t free_observed_guard = 0;
    int manage_calls = 0;
    int virtual_calls = 0;
    int break_calls = 0;
    int free_calls = 0;
};

class IntegratedMemory final : public Memory, public pal4::IHeapRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        return Memory::Read(address);
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        Memory::Write(address, value);
    }

    void ManageBlocks(const std::uint32_t address) override {
        Memory::ManageBlocks(address);
    }

    [[nodiscard]] std::uint32_t Extend(
        const std::uint32_t size,
        const std::uint32_t heap_address) override {
        return pal4::HeapExtender::Allocate(size, heap_address, *this);
    }
};

void SeedRegularHeap(Memory& memory, const std::uint32_t top_size) {
    memory.Write(kHeap, 1u);
    memory.Write(kHeap + 44u, 0x1000u);
    memory.Write(0x1004u, top_size | 1u);
    memory.Write(kHeap + 860u, 0x1000u);
    memory.Write(kHeap + 840u, 0x1000u);
    memory.Write(kHeap + 864u, 1u);
}

void CheckConsolidationRetry() {
    Memory memory;
    memory.Write(kHeap, 3u);
    memory.Write(kHeap + 860u, 0x1000u);
    Check(pal4::HeapExtender::Allocate(40u, kHeap, memory) == 0x1234u,
          "fastbin consolidation retries original allocation");
    Check(memory.manage_calls == 1 && memory.retry_request == 33u,
          "retry uses block size minus seven");
    Check(memory.virtual_calls == 0 && memory.break_calls == 0,
          "retry does not request new pages directly");
}

void CheckMappedAlignedAndMisaligned() {
    Memory aligned;
    aligned.Write(kHeap + 860u, 0x1000u);
    aligned.Write(kHeap + 844u, 0x100u);
    aligned.Write(kHeap + 852u, 2u);
    aligned.virtual_results.push_back(0x4000u);
    Check(pal4::HeapExtender::Allocate(0x200u, kHeap, aligned) == 0x4008u,
          "mapped allocation returns aligned payload");
    Check(aligned.virtual_size == 0x1000u &&
              aligned.Read(0x4004u) == 0x1002u,
          "mapped header records page span and mapping flag");
    Check(aligned.Read(kHeap + 848u) == 1u &&
              aligned.Read(kHeap + 856u) == 1u &&
              aligned.Read(kHeap + 868u) == 0x1000u,
          "mapping counters and high-water marks advance");
    Check(aligned.Read(0x8dd834u) == 0xff8u,
          "mapped allocation uses mapped payload accounting");

    Memory misaligned;
    misaligned.Write(kHeap + 860u, 0x1000u);
    misaligned.Write(kHeap + 844u, 0x100u);
    misaligned.Write(kHeap + 852u, 1u);
    misaligned.virtual_results.push_back(0x5003u);
    Check(pal4::HeapExtender::Allocate(0x200u, kHeap, misaligned) == 0x5010u,
          "misaligned mapping advances chunk to eight-byte boundary");
    Check(misaligned.Read(0x5008u) == 5u &&
              misaligned.Read(0x500cu) == 0xffbu,
          "misaligned mapping stores padding and adjusted size");
}

void CheckContiguousProgramBreak() {
    Memory memory;
    SeedRegularHeap(memory, 0x100u);
    memory.break_results.push_back(0x1100u);
    Check(pal4::HeapExtender::Allocate(0x28u, kHeap, memory) == 0x1008u,
          "contiguous extension enlarges and splits old top");
    Check(memory.break_calls == 1 && memory.last_break_size == 0x1000u,
          "program break receives granularity-aligned span");
    Check(memory.Read(kHeap + 44u) == 0x1028u &&
              memory.Read(0x102cu) == 0x10d9u,
          "contiguous extension writes new top remainder");
    Check(memory.Read(kHeap + 872u) == 0x1000u &&
              memory.Read(0x8dd834u) == 0x24u,
          "ordinary heap and payload counters advance");
}

void CheckVirtualFallback() {
    Memory memory;
    SeedRegularHeap(memory, 0x100u);
    memory.break_results.push_back(kFailure);
    memory.virtual_results.push_back(0x8000u);
    Check(pal4::HeapExtender::Allocate(0x28u, kHeap, memory) == 0x8008u,
          "failed program break falls back to virtual region");
    Check(memory.virtual_size >= 0x100000u,
          "fallback requests at least one megabyte");
    Check((memory.Read(kHeap + 864u) & 1u) == 0u,
          "virtual fallback clears contiguous-morecore flag");
    Check(memory.free_calls == 1 && memory.freed_payload == 0x1008u &&
              memory.free_observed_guard == kFailure,
          "old top is freed under the observed temporary guard");
    Check(memory.Read(kHeap + 836u) == 0u,
          "old guard is restored after free");
}

void CheckAllocatorIntegration() {
    IntegratedMemory memory;
    SeedRegularHeap(memory, 0x20u);
    memory.Write(kHeap, 4u);
    memory.break_results.push_back(0x1020u);
    Check(pal4::HeapAllocator::Allocate(32u, memory) == 0x1008u,
          "allocator delegates undersized top to reconstructed extender");
    Check(memory.break_calls == 1 && memory.Read(kHeap + 44u) == 0x1028u,
          "extension and allocation share the same top pointer");
    Check(memory.Read(0x8dd834u) == 36u,
          "delegated allocation is accounted once");
}

}  // namespace

int main() {
    CheckConsolidationRetry();
    CheckMappedAlignedAndMisaligned();
    CheckContiguousProgramBreak();
    CheckVirtualFallback();
    CheckAllocatorIntegration();
    std::cout << "5 heap extender test cases passed\n";
}
