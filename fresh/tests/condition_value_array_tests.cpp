#include "pal4/condition_value_array.h"

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

struct Runtime final : pal4::IConditionValueArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    pal4::ConditionValueArrayState* state{};
    bool truncate_after_first{};
    std::vector<std::uint32_t> writes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_allocation = 0x4000u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        if (truncate_after_first)
            state->end = state->begin + 4u;
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

struct IteratorRuntime final : pal4::IConditionValueIteratorRuntime {
    std::uint32_t allocation_result = 0x9000u;
    std::uint32_t allocated_bytes{};
    std::uint32_t stored_address{};
    std::uint32_t output_address{};
    std::uint32_t output_value{};
    pal4::ConditionValueIteratorImage image{};
    std::vector<int> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back(1);
        allocated_bytes = bytes;
        return allocation_result;
    }
    void Store(std::uint32_t address,
               const pal4::ConditionValueIteratorImage& value) override {
        calls.push_back(2);
        stored_address = address;
        image = value;
    }
    void WriteOutput(std::uint32_t address, std::uint32_t value) override {
        calls.push_back(3);
        output_address = address;
        output_value = value;
    }
};

void TestEmptyAndMissing() {
    pal4::ConditionValueArrayState state;
    Runtime runtime;
    Check(!pal4::ConditionValueArray::Contains(state, 7u, runtime) &&
              runtime.reads.empty(),
          "null begin stops before reading memory");
    state.begin = 0x1000u;
    state.end = 0x1000u;
    Check(!pal4::ConditionValueArray::Contains(state, 7u, runtime) &&
              runtime.reads.empty(),
          "empty non-null array also stops before reading");
    state.end = 0x100cu;
    runtime.words = {{0x1000u, 1u}, {0x1004u, 2u}, {0x1008u, 3u}};
    Check(!pal4::ConditionValueArray::Contains(state, 7u, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1000u, 0x1004u, 0x1008u},
          "missing value visits all three DWORDs");
}

void TestFirstLastAndReload() {
    pal4::ConditionValueArrayState state;
    state.begin = 0x2000u;
    state.end = 0x200cu;
    Runtime runtime;
    runtime.words = {{0x2000u, 5u}, {0x2004u, 6u}, {0x2008u, 7u}};
    Check(pal4::ConditionValueArray::Contains(state, 5u, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{0x2000u},
          "first match returns immediately");
    runtime.reads.clear();
    Check(pal4::ConditionValueArray::Contains(state, 7u, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x2000u, 0x2004u, 0x2008u},
          "last match uses four-byte stride");
    runtime.reads.clear();
    runtime.state = &state;
    runtime.truncate_after_first = true;
    Check(!pal4::ConditionValueArray::Contains(state, 7u, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{0x2000u},
          "array end is reloaded after each unsuccessful read");
}

void TestNoOpAndRemoval() {
    pal4::ConditionValueArrayState state;
    state.begin = 0x1000u;
    state.end = 0x100cu;
    state.capacity_end = 0x1010u;
    Runtime runtime;
    runtime.words = {{0x1000u, 5u}, {0x1004u, 6u}, {0x1008u, 7u}};
    Check(pal4::ConditionValueArray::SetPresent(
              state, 6u, 0x101u, runtime) == 0x1001u &&
              state.end == 0x100cu && runtime.writes.empty(),
          "present enabled value keeps array and returns modified end word");
    runtime.reads.clear();
    Check(pal4::ConditionValueArray::SetPresent(
              state, 8u, 0x100u, runtime) == 0x1000u &&
              state.end == 0x100cu && runtime.writes.empty(),
          "missing disabled value performs no write");
    runtime.reads.clear();
    Check(pal4::ConditionValueArray::SetPresent(
              state, 5u, 0u, runtime) == 0x1008u &&
              state.end == 0x1008u && runtime.words[0x1000u] == 6u &&
              runtime.words[0x1004u] == 7u &&
              runtime.writes == std::vector<std::uint32_t>{0x1000u, 0x1004u},
          "removing first match shifts all following DWORDs left");
    runtime.writes.clear();
    Check(pal4::ConditionValueArray::SetPresent(
              state, 7u, 0u, runtime) == 0x1004u &&
              state.end == 0x1004u && runtime.writes.empty(),
          "removing last match only moves the end pointer");
}

void TestAppendWithCapacity() {
    pal4::ConditionValueArrayState state;
    state.begin = 0x2000u;
    state.end = 0x2004u;
    state.capacity_end = 0x200cu;
    Runtime runtime;
    runtime.words[0x2000u] = 3u;
    Check(pal4::ConditionValueArray::SetPresent(
              state, 9u, 1u, runtime) == 0x2004u &&
              state.end == 0x2008u && state.capacity_end == 0x200cu &&
              runtime.words[0x2004u] == 9u &&
              runtime.allocations.empty() && runtime.frees.empty(),
          "spare capacity appends in place and returns prior end");
}

void TestGrowthFromEmptyAndPopulated() {
    pal4::ConditionValueArrayState state;
    Runtime runtime;
    Check(pal4::ConditionValueArray::SetPresent(
              state, 11u, 1u, runtime) == 0x4004u &&
              state.begin == 0x4000u && state.end == 0x4004u &&
              state.capacity_end == 0x4004u &&
              runtime.allocations == std::vector<std::uint32_t>{4u} &&
              runtime.frees == std::vector<std::uint32_t>{0u} &&
              runtime.words[0x4000u] == 11u,
          "empty array allocates one DWORD and frees old null begin");

    runtime.next_allocation = 0x5000u;
    runtime.allocations.clear();
    runtime.frees.clear();
    Check(pal4::ConditionValueArray::SetPresent(
              state, 12u, 1u, runtime) == 0x5008u &&
              state.begin == 0x5000u && state.end == 0x5008u &&
              state.capacity_end == 0x5008u &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.frees == std::vector<std::uint32_t>{0x4000u} &&
              runtime.words[0x5000u] == 11u &&
              runtime.words[0x5004u] == 12u,
          "single entry grows to two DWORDs and copies the old element");

    runtime.next_allocation = 0x6000u;
    runtime.allocations.clear();
    runtime.frees.clear();
    Check(pal4::ConditionValueArray::SetPresent(
              state, 13u, 1u, runtime) == 0x600cu &&
              state.begin == 0x6000u && state.end == 0x600cu &&
              state.capacity_end == 0x6010u &&
              runtime.allocations == std::vector<std::uint32_t>{16u} &&
              runtime.frees == std::vector<std::uint32_t>{0x5000u} &&
              runtime.words[0x6000u] == 11u &&
              runtime.words[0x6004u] == 12u &&
              runtime.words[0x6008u] == 13u,
          "two entries double capacity to four DWORDs");
}

void TestIteratorCreation() {
    pal4::ConditionValueArrayState state;
    state.begin = 0x5000u;
    IteratorRuntime runtime;
    Check(pal4::ConditionValueArray::GetIterator(
              state, 0x3000u, 0xa000u, runtime) == 0xa000u &&
              runtime.allocated_bytes == 12u &&
              runtime.stored_address == 0x9000u &&
              runtime.image.vtable ==
                  pal4::ConditionValueArray::kIteratorVtable &&
              runtime.image.current == 0x5000u &&
              runtime.image.owner == 0x30b0u &&
              runtime.output_address == 0xa000u &&
              runtime.output_value == 0x9000u &&
              runtime.calls == std::vector<int>{1, 2, 3},
          "iterator allocation initializes image before output write");

    runtime = {};
    runtime.allocation_result = 0u;
    Check(pal4::ConditionValueArray::GetIterator(
              state, 0x3000u, 0xa000u, runtime) == 0xa000u &&
              runtime.output_value == 0u &&
              runtime.calls == std::vector<int>{1, 3},
          "allocation failure stores null without initializing an iterator");
    pal4::ConditionValueIteratorRuntimeStub stub;
    Check(pal4::ConditionValueArray::GetIterator(
              state, 0x3000u, 0xa000u, stub) == 0xa000u,
          "default iterator runtime links");
}

}  // namespace

int main() {
    TestEmptyAndMissing();
    TestFirstLastAndReload();
    TestNoOpAndRemoval();
    TestAppendWithCapacity();
    TestGrowthFromEmptyAndPopulated();
    TestIteratorCreation();
    pal4::ConditionValueArrayState state;
    pal4::ConditionValueArrayRuntimeStub stub;
    Check(!pal4::ConditionValueArray::Contains(state, 0u, stub),
          "default memory runtime links");
    std::cout << "10 condition value array cases passed\n";
}
