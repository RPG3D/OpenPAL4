#include "pal4/indexed_key_value_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IIndexedKeyValueCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> writes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_allocation = 0x4000u;
    pal4::IndexedKeyValueCollectionState* state{};
    bool shorten_after_first{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        if (shorten_after_first)
            state->slots[5].end = state->slots[5].begin + 8u;
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return next_allocation;
    }
    void Free(std::uint32_t address) override { frees.push_back(address); }
};

using Collection = pal4::IndexedKeyValueCollection;
using State = pal4::IndexedKeyValueCollectionState;

void TestInvalidIndicesAndExistingKey() {
    State state;
    Runtime runtime;
    Collection::AddDelta(state, -1, 3, 9u, runtime);
    Collection::AddDelta(state, 6, 3, 9u, runtime);
    Check(runtime.reads.empty() && runtime.writes.empty() &&
              runtime.allocations.empty(),
          "indices below zero and above five do not touch memory");

    state.slots[5] = {0x55u, 0x3000u, 0x3018u, 0x3020u};
    runtime.words = {{0x3000u, static_cast<std::uint32_t>(-4)},
                     {0x3004u, 10u}, {0x3008u, 2u},
                     {0x300cu, 20u}, {0x3010u, 9u},
                     {0x3014u, 0xfffffffeu}};
    Collection::AddDelta(state, 5, 9, 5u, runtime);
    Check(runtime.words[0x3014u] == 3u &&
              runtime.writes == std::vector<std::uint32_t>{0x3014u} &&
              runtime.allocations.empty() && state.slots[5].end == 0x3018u &&
              state.slots[5].reserved == 0x55u,
          "existing key updates only its value with 32-bit wraparound");
}

void TestInPlaceInsertion() {
    State state;
    state.slots[2] = {0x99u, 0x2000u, 0x2018u, 0x2030u};
    Runtime runtime;
    runtime.words = {{0x2000u, static_cast<std::uint32_t>(-4)},
                     {0x2004u, 4u}, {0x2008u, 2u}, {0x200cu, 5u},
                     {0x2010u, 9u}, {0x2014u, 6u}};
    Collection::AddDelta(state, 2, 4, 11u, runtime);
    Check(state.slots[2].end == 0x2020u &&
              state.slots[2].capacity_end == 0x2030u &&
              runtime.words[0x2010u] == 4u &&
              runtime.words[0x2014u] == 11u &&
              runtime.words[0x2018u] == 9u &&
              runtime.words[0x201cu] == 6u &&
              runtime.allocations.empty(),
          "middle insertion shifts a complete key/value pair");
    Collection::AddDelta(state, 2, -7, 3u, runtime);
    Check(state.slots[2].end == 0x2028u &&
              runtime.words[0x2000u] == static_cast<std::uint32_t>(-7) &&
              runtime.words[0x2004u] == 3u &&
              runtime.words[0x2008u] == static_cast<std::uint32_t>(-4) &&
              runtime.words[0x200cu] == 4u,
          "negative key inserts at the head using signed comparison");
    Collection::AddDelta(state, 2, 12, 8u, runtime);
    Check(state.slots[2].end == 0x2030u &&
              runtime.words[0x2028u] == 12u &&
              runtime.words[0x202cu] == 8u &&
              runtime.allocations.empty() && state.slots[2].reserved == 0x99u,
          "tail insertion uses remaining capacity and keeps metadata");
}

void TestGrowthAndOrder() {
    State state;
    Runtime runtime;
    Collection::AddDelta(state, 0, 3, 30u, runtime);
    Check(state.slots[0].begin == 0x4000u &&
              state.slots[0].end == 0x4008u &&
              state.slots[0].capacity_end == 0x4008u &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.frees == std::vector<std::uint32_t>{0u} &&
              runtime.words[0x4000u] == 3u && runtime.words[0x4004u] == 30u,
          "empty key/value slot allocates one pair");

    runtime.next_allocation = 0x5000u;
    runtime.allocations.clear();
    runtime.frees.clear();
    Collection::AddDelta(state, 0, 1, 10u, runtime);
    Check(state.slots[0].begin == 0x5000u &&
              state.slots[0].end == 0x5010u &&
              state.slots[0].capacity_end == 0x5010u &&
              runtime.allocations == std::vector<std::uint32_t>{16u} &&
              runtime.frees == std::vector<std::uint32_t>{0x4000u} &&
              runtime.words[0x5000u] == 1u && runtime.words[0x5004u] == 10u &&
              runtime.words[0x5008u] == 3u && runtime.words[0x500cu] == 30u,
          "one-pair growth inserts before existing pair");

    runtime.next_allocation = 0x6000u;
    runtime.allocations.clear();
    runtime.frees.clear();
    Collection::AddDelta(state, 0, 2, 20u, runtime);
    Check(state.slots[0].begin == 0x6000u &&
              state.slots[0].end == 0x6018u &&
              state.slots[0].capacity_end == 0x6020u &&
              runtime.allocations == std::vector<std::uint32_t>{32u} &&
              runtime.frees == std::vector<std::uint32_t>{0x5000u} &&
              runtime.words[0x6000u] == 1u && runtime.words[0x6004u] == 10u &&
              runtime.words[0x6008u] == 2u && runtime.words[0x600cu] == 20u &&
              runtime.words[0x6010u] == 3u && runtime.words[0x6014u] == 30u,
          "two-pair growth doubles capacity and preserves both sides");
}

void TestValueLookup() {
    State state;
    Runtime runtime;
    Check(Collection::GetValue(state, -1, 3, runtime) == 0u &&
              Collection::GetValue(state, 6, 3, runtime) == 0u &&
              Collection::GetValue(state, 0, 3, runtime) == 0u &&
              runtime.reads.empty(),
          "invalid indices and empty slot return zero without memory reads");
    state.slots[5] = {0u, 0x7000u, 0x7018u, 0x7018u};
    runtime.words = {{0x7000u, static_cast<std::uint32_t>(-4)},
                     {0x7004u, 40u}, {0x7008u, 2u}, {0x700cu, 20u},
                     {0x7010u, 9u}, {0x7014u, 90u}};
    Check(Collection::GetValue(state, 5, -4, runtime) == 40u &&
              Collection::GetValue(state, 5, 2, runtime) == 20u &&
              Collection::GetValue(state, 5, 9, runtime) == 90u,
          "signed lookup returns values from first, middle and last pairs");
    for (int key : {-5, -1, 3, 10})
        Check(Collection::GetValue(state, 5, key, runtime) == 0u,
              "lookup returns zero for gaps and values outside the range");
    Check(runtime.writes.empty() && runtime.allocations.empty(),
          "lookup does not modify the collection");
}

void TestTotalValue() {
    State state;
    Runtime runtime;
    Check(Collection::TotalValue(state, -1, runtime) == 0u &&
              Collection::TotalValue(state, 6, runtime) == 0u &&
              Collection::TotalValue(state, 5, runtime) == 0u &&
              runtime.reads.empty(),
          "invalid index and null begin return zero without memory reads");
    state.slots[5] = {0x55u, 0x7000u, 0x7018u, 0x7020u};
    runtime.words = {{0x7004u, 0xfffffffdu},
                     {0x700cu, 4u}, {0x7014u, 6u}};
    Check(Collection::TotalValue(state, 5, runtime) == 7u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x7004u, 0x700cu, 0x7014u} &&
              state.slots[5].reserved == 0x55u,
          "total sums second DWORD of each pair with 32-bit wraparound");
    runtime.reads.clear();
    runtime.state = &state;
    runtime.shorten_after_first = true;
    Check(Collection::TotalValue(state, 5, runtime) == 0xfffffffdu &&
              runtime.reads == std::vector<std::uint32_t>{0x7004u},
          "total reloads the end pointer after each value read");
}

void TestRemainingValue() {
    State state;
    Runtime runtime;
    state.slots[0] = {0u, 0x1000u, 0x1008u, 0x1008u};
    state.slots[3] = {0u, 0x3000u, 0x3010u, 0x3010u};
    state.slots[5] = {0u, 0x5000u, 0x5008u, 0x5008u};
    runtime.words = {{0x1004u, 2u}, {0x3004u, 3u},
                     {0x300cu, 4u}, {0x5004u, 999u}};
    Check(Collection::RemainingValue(state, 20u, runtime) == 11u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1004u, 0x3004u, 0x300cu},
          "remaining value subtracts totals from slots zero through four only");
    runtime.reads.clear();
    Check(Collection::RemainingValue(state, 1u, runtime) ==
              0xfffffff8u && runtime.reads.size() == 3u,
          "remaining value preserves 32-bit subtraction wraparound");
}

}  // namespace

int main() {
    TestInvalidIndicesAndExistingKey();
    TestInPlaceInsertion();
    TestGrowthAndOrder();
    TestValueLookup();
    TestTotalValue();
    TestRemainingValue();
    State state;
    pal4::IndexedKeyValueCollectionRuntimeStub stub;
    Collection::AddDelta(state, -1, 0, 0u, stub);
    std::cout << "7 indexed key/value collection cases passed\n";
}
