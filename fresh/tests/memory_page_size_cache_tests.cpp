#include "pal4/memory_page_size_cache.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Platform final : public pal4::ISystemPageInfo {
public:
    [[nodiscard]] std::uint32_t GetSystemPageSize() override {
        ++calls;
        return response;
    }

    std::uint32_t response = 4096u;
    int calls = 0;
};

void CheckCacheHit() {
    pal4::MemoryPageSizeCache cache;
    Platform platform;
    Check(cache.Get(platform) == 4096u && platform.calls == 1,
          "first lookup queries system page size");
    platform.response = 8192u;
    Check(cache.Get(platform) == 4096u && platform.calls == 1 &&
              cache.CachedValue() == 4096u,
          "nonzero cached value bypasses system call");
}

void CheckZeroResponseRetries() {
    pal4::MemoryPageSizeCache cache;
    Platform platform;
    platform.response = 0u;
    Check(cache.Get(platform) == 0u && platform.calls == 1,
          "zero page size is stored as zero");
    platform.response = 16384u;
    Check(cache.Get(platform) == 16384u && platform.calls == 2,
          "zero cache causes a later system query");
}

}  // namespace

int main() {
    CheckCacheHit();
    CheckZeroResponseRetries();
    std::cout << "2 memory page size cache test cases passed\n";
}
