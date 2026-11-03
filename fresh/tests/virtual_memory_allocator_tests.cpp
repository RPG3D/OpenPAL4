#include "pal4/virtual_memory_allocator.h"

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

class Runtime final : public pal4::IVirtualMemoryAllocatorRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    [[nodiscard]] std::uint32_t PageSize() override {
        events.emplace_back("page");
        return page_result;
    }

    [[nodiscard]] std::uint32_t AllocationGranularity() override {
        events.emplace_back("granularity");
        return granularity_result;
    }

    [[nodiscard]] std::uint32_t VirtualAllocate(
        const std::uint32_t address, const std::uint32_t size,
        const std::uint32_t type,
        const std::uint32_t protection) override {
        events.emplace_back("allocate");
        Check(address == 0x10000u && size == 0x2000u &&
                  type == 0x103000u && protection == 4u,
              "VirtualAlloc receives address, size, flags and protection");
        return allocate_result;
    }

    [[nodiscard]] bool VirtualRelease(
        const std::uint32_t address, const std::uint32_t size,
        const std::uint32_t type) override {
        events.emplace_back("release");
        Check(address == 0x10000u && size == 0u && type == 0x8000u,
              "VirtualFree releases the entire reserved region");
        return release_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t page_result = 0x1000u;
    std::uint32_t granularity_result = 0x10000u;
    std::uint32_t allocate_result = 0x10000u;
    bool release_result = true;
};

void CheckAllocationAndFailure() {
    Runtime runtime;
    Check(pal4::VirtualMemoryAllocator::Allocate(
              0x10000u, 0x2000u, 3u, 3u, 0xffffffffu, 0u,
              runtime) == 0x10000u,
          "successful allocation returns VirtualAlloc address");
    Check(runtime.Read(0x8dd818u) == 0x1000u &&
              runtime.Read(0x8dd828u) == 0x10000u &&
              runtime.events == std::vector<std::string>{
                  "page", "granularity", "allocate"},
          "allocation lazily initializes its own caches before VirtualAlloc");
    runtime.allocate_result = 0u;
    Check(pal4::VirtualMemoryAllocator::Allocate(
              0x10000u, 0x2000u, 1u, 2u, 7u, 9u, runtime) ==
              pal4::VirtualMemoryAllocator::kFailure,
          "null VirtualAlloc result maps to minus one");
    Check(runtime.events == std::vector<std::string>{
              "page", "granularity", "allocate", "allocate"},
          "warm caches avoid repeated system information calls");
}

void CheckReleaseAndIndependentCaches() {
    Runtime runtime;
    Check(pal4::VirtualMemoryAllocator::Allocate(
              0x10000u, 0x2000u, 3u, 3u, 0xffffffffu, 0u,
              runtime) == 0x10000u,
          "setup allocation succeeds");
    Check(pal4::VirtualMemoryAllocator::Release(
              0x10000u, 0x2000u, runtime) == 0u,
          "successful VirtualFree maps to zero");
    Check(runtime.Read(0x8dd830u) == 0x1000u &&
              runtime.Read(0x8dd82cu) == 0x10000u &&
              runtime.events == std::vector<std::string>{
                  "page", "granularity", "allocate", "page",
                  "granularity", "release"},
          "release uses a separate pair of cache slots");
    runtime.release_result = false;
    Check(pal4::VirtualMemoryAllocator::Release(0x10000u, 0x3000u, runtime) ==
              pal4::VirtualMemoryAllocator::kFailure,
          "failed VirtualFree maps to minus one");
    Check(runtime.events.back() == "release" && runtime.events.size() == 7u,
          "warm release caches avoid system queries");
}

void CheckZeroCacheValuesAreRetried() {
    Runtime runtime;
    runtime.page_result = 0u;
    runtime.granularity_result = 0u;
    (void)pal4::VirtualMemoryAllocator::Release(0x10000u, 0x2000u, runtime);
    runtime.page_result = 0x2000u;
    runtime.granularity_result = 0x20000u;
    (void)pal4::VirtualMemoryAllocator::Release(0x10000u, 0x3000u, runtime);
    Check(runtime.Read(0x8dd830u) == 0x2000u &&
              runtime.Read(0x8dd82cu) == 0x20000u &&
              runtime.events == std::vector<std::string>{
                  "page", "granularity", "release", "page",
                  "granularity", "release"},
          "zero cache entries trigger another system query");
}

}  // namespace

int main() {
    CheckAllocationAndFailure();
    CheckReleaseAndIndependentCaches();
    CheckZeroCacheValuesAreRetried();
    std::cout << "3 virtual memory allocator test cases passed\n";
}
