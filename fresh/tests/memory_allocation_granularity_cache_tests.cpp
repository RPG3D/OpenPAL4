#include "pal4/memory_allocation_granularity_cache.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Platform final : public pal4::ISystemAllocationGranularity {
public:
    [[nodiscard]] std::uint32_t GetSystemAllocationGranularity() override {
        ++calls;
        return response;
    }

    std::uint32_t response = 65536u;
    int calls = 0;
};

void CheckCacheAndZeroRetry() {
    pal4::MemoryAllocationGranularityCache cache;
    Platform platform;
    Check(cache.Get(platform) == 65536u && platform.calls == 1,
          "initial lookup obtains system allocation granularity");
    platform.response = 131072u;
    Check(cache.Get(platform) == 65536u && platform.calls == 1 &&
              cache.CachedValue() == 65536u,
          "nonzero granularity is returned without another system call");

    pal4::MemoryAllocationGranularityCache zero_cache;
    platform.response = 0u;
    Check(zero_cache.Get(platform) == 0u && platform.calls == 2,
          "zero response remains a cache miss");
    platform.response = 4096u;
    Check(zero_cache.Get(platform) == 4096u && platform.calls == 3,
          "later lookup retries after a zero response");
}

}  // namespace

int main() {
    CheckCacheAndZeroRetry();
    std::cout << "1 allocation granularity cache test case passed\n";
}
