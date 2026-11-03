#include "pal4/heap_bin_state.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {

constexpr std::uint32_t kHeap = 0x8dd4a0u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IHeapBinStateRuntime {
public:
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    [[nodiscard]] std::uint32_t PageSize() override {
        ++page_size_calls;
        return 0x2000u;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    int page_size_calls = 0;
};

void CheckSentinelsAndConfiguration() {
    Memory memory;
    memory.Write(kHeap, 2u);
    memory.Write(kHeap + 0x360u, 0x80u);
    Check(pal4::HeapBinState::Initialize(kHeap, memory) == 0x2000u,
          "initializer returns platform page size");
    for (std::uint32_t index = 0; index < 95u; ++index) {
        const std::uint32_t sentinel = kHeap + 0x34u + index * 8u;
        Check(memory.Read(sentinel + 8u) == sentinel &&
                  memory.Read(sentinel + 12u) == sentinel,
              "all 95 bin sentinels link to themselves");
    }
    Check(memory.Read(kHeap + 0x2cu) == kHeap + 0x34u &&
              memory.Read(kHeap) == 0x4au,
          "top and heap state flags are initialized");
    Check(memory.Read(kHeap + 0x344u) == 0x40000u &&
              memory.Read(kHeap + 0x348u) == 0u &&
              memory.Read(kHeap + 0x34cu) == 0x40000u &&
              memory.Read(kHeap + 0x354u) == 0x10000u,
          "observed heap thresholds are written");
    Check(memory.Read(kHeap + 0x360u) == 0x81u &&
              memory.Read(kHeap + 0x35cu) == 0x2000u &&
              memory.page_size_calls == 1,
          "page-size state and flag bit are preserved correctly");
}

}  // namespace

int main() {
    CheckSentinelsAndConfiguration();
    std::cout << "1 heap bin state test case passed\n";
}
