#include "pal4/memory_region_chain.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMemoryRegionChainRuntime {
public:
    [[nodiscard]] std::uint32_t GetProcessHeap() override {
        events.emplace_back("heap");
        return 0x8000u;
    }

    [[nodiscard]] std::uint32_t HeapAllocate(
        const std::uint32_t heap, const std::uint32_t flags,
        const std::uint32_t bytes) override {
        events.emplace_back("alloc");
        Check(heap == 0x8000u && flags == 0u && bytes == 20u,
              "allocate uses process heap, zero flags and 20 bytes");
        return allocation_result;
    }

    [[nodiscard]] std::uint32_t HeapFree(
        const std::uint32_t heap, const std::uint32_t flags,
        const std::uint32_t address) override {
        events.emplace_back("free");
        Check(heap == 0x8000u && flags == 0u && address == expected_free,
              "free uses process heap and current chain head");
        return free_result;
    }

    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        events.emplace_back("write");
        words[address] = value;
    }

    std::uint32_t allocation_result = 0x9000u;
    std::uint32_t free_result = 1u;
    std::uint32_t expected_free = 0x9000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
};

void CheckPushInitializationAndLinking() {
    Runtime runtime;
    std::uint32_t head = 0x7000u;
    pal4::MemoryRegionChain chain(head);
    Check(chain.Push(0x10000u, 0x20000u, runtime) == 1u,
          "successful allocation returns one");
    Check(head == 0x9000u && runtime.Read(0x9000u) == 0x10000u &&
              runtime.Read(0x9004u) == 0x10000u &&
              runtime.Read(0x9008u) == 0x30000u &&
              runtime.Read(0x900cu) == 0x20000u &&
              runtime.Read(0x9010u) == 0x7000u,
          "descriptor fields and previous head follow IDA layout");
    Check(runtime.events == std::vector<std::string>{
              "heap", "alloc", "write", "write", "write", "write", "write"},
          "heap allocation precedes all descriptor writes");

    runtime.allocation_result = 0xa000u;
    Check(chain.Push(0xfffff000u, 0x2000u, runtime) == 1u &&
              head == 0xa000u && runtime.Read(0xa008u) == 0x1000u &&
              runtime.Read(0xa010u) == 0x9000u,
          "second descriptor links to first and end wraps at 32 bits");
}

void CheckPushFailureDoesNotPublish() {
    Runtime runtime;
    runtime.allocation_result = 0u;
    std::uint32_t head = 0x7000u;
    Check(pal4::MemoryRegionChain(head).Push(1u, 2u, runtime) == 0u &&
              head == 0x7000u && runtime.words.empty(),
          "failed allocation leaves head and descriptor memory intact");
    Check(runtime.events == std::vector<std::string>{"heap", "alloc"},
          "allocation failure performs no descriptor writes");
}

void CheckPopSuccessAndFailure() {
    Runtime runtime;
    std::uint32_t head = 0x9000u;
    runtime.words[0x9010u] = 0x7000u;
    pal4::MemoryRegionChain chain(head);
    runtime.free_result = 0u;
    Check(chain.Pop(runtime) == 0u && head == 0x9000u,
          "failed HeapFree keeps current head");
    runtime.free_result = 7u;
    Check(chain.Pop(runtime) == 1u && head == 0x7000u,
          "nonzero HeapFree result updates head and returns one");
    Check(runtime.events == std::vector<std::string>{
              "heap", "free", "heap", "free"},
          "each pop obtains the process heap before freeing");
}

}  // namespace

int main() {
    CheckPushInitializationAndLinking();
    CheckPushFailureDoesNotPublish();
    CheckPopSuccessAndFailure();
    std::cout << "3 memory region chain test cases passed\n";
}
