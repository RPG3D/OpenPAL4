#include "pal4/heap_trim.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kHeap = 0x8dd4a0u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IHeapTrimRuntime {
public:
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    [[nodiscard]] std::uint32_t ExtendProgramBreak(
        const std::uint32_t amount) override {
        last_amount = amount;
        amounts.push_back(amount);
        ++calls;
        if (results.empty()) {
            return 0xffffffffu;
        }
        const std::uint32_t result = results.front();
        results.pop_front();
        return result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::deque<std::uint32_t> results;
    std::uint32_t last_amount = 0;
    std::vector<std::uint32_t> amounts;
    int calls = 0;
};

void Seed(Memory& memory, const std::uint32_t size) {
    memory.Write(kHeap + 44u, 0x100000u);
    memory.Write(kHeap + 860u, 0x1000u);
    memory.Write(kHeap + 872u, size);
    memory.Write(0x100004u, size | 1u);
}

void CheckInsufficientSpace() {
    Memory memory;
    Seed(memory, 0x1000u);
    Check(!pal4::HeapTrim::Adjust(0x1000u, kHeap, memory),
          "no whole page beyond padding is released");
    Check(memory.calls == 0, "no program-break call for small remainder");
}

void CheckNoncontiguousBreak() {
    Memory memory;
    Seed(memory, 0x5000u);
    memory.results.push_back(0x900000u);
    Check(!pal4::HeapTrim::Adjust(0x1000u, kHeap, memory),
          "noncontiguous program break prevents trimming");
    Check(memory.calls == 1 && memory.Read(0x100004u) == 0x5001u,
          "noncontiguous case preserves top header");
}

void CheckSuccessfulPartialTrim() {
    Memory memory;
    Seed(memory, 0x5000u);
    memory.results.push_back(0x105000u);
    memory.results.push_back(0x105000u);
    memory.results.push_back(0x103000u);
    Check(pal4::HeapTrim::Adjust(0x1000u, kHeap, memory),
          "positive returned shrink updates heap");
    Check(memory.calls == 3 && memory.Read(0x100004u) == 0x3001u,
          "top header follows actual shrink, not requested shrink");
    Check(memory.amounts[1] == 0xffffd000u,
          "trim requests three negative pages before measuring actual shrink");
    Check(memory.Read(kHeap + 872u) == 0x3000u,
          "heap byte counter follows actual shrink");
}

void CheckFailedShrinkQuery() {
    Memory memory;
    Seed(memory, 0x5000u);
    memory.results.push_back(0x105000u);
    memory.results.push_back(0xffffffffu);
    memory.results.push_back(0xffffffffu);
    Check(!pal4::HeapTrim::Adjust(0x1000u, kHeap, memory),
          "failed post-shrink query leaves metadata unchanged");
    Check(memory.Read(0x100004u) == 0x5001u &&
              memory.Read(kHeap + 872u) == 0x5000u,
          "failure preserves header and byte count");
}

}  // namespace

int main() {
    CheckInsufficientSpace();
    CheckNoncontiguousBreak();
    CheckSuccessfulPartialTrim();
    CheckFailedShrinkQuery();
    std::cout << "4 heap trim test cases passed\n";
}
