#include "pal4/indexed_sorted_value_arrays.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IIndexedSortedValueArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> writes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_allocation = 0x4000u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
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

struct IteratorRuntime final : pal4::IIndexedSortedIteratorRuntime {
    std::uint32_t next_allocation = 0x9000u;
    std::uint32_t allocated_bytes{};
    std::uint32_t stored_address{};
    std::uint32_t output_address{};
    std::uint32_t output_value{};
    pal4::IndexedSortedIteratorImage valid{};
    pal4::InvalidSortedIteratorImage invalid{};
    std::vector<int> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back(1);
        allocated_bytes = bytes;
        return next_allocation;
    }
    void StoreValid(std::uint32_t address,
                    const pal4::IndexedSortedIteratorImage& image) override {
        calls.push_back(2);
        stored_address = address;
        valid = image;
    }
    void StoreInvalid(std::uint32_t address,
                      const pal4::InvalidSortedIteratorImage& image) override {
        calls.push_back(3);
        stored_address = address;
        invalid = image;
    }
    void WriteOutput(std::uint32_t address, std::uint32_t value) override {
        calls.push_back(4);
        output_address = address;
        output_value = value;
    }
};

struct ProcessRuntime final : pal4::ISortedValueProcessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::FixedSortedValueArrayState* state{};
    bool shorten_after_first{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void Process(std::uint32_t context, std::uint32_t value) override {
        calls.push_back("process:" + std::to_string(context) + ":" +
                        std::to_string(value));
        if (shorten_after_first)
            state->values.end = state->values.begin + 4u;
    }
};

using Arrays = pal4::IndexedSortedValueArrays;
using State = pal4::IndexedSortedValueArrayState;

void TestIndexGatesAndEmptySlot() {
    State state;
    Runtime runtime;
    Check(Arrays::InsertUnique(state, -1, 9, runtime) == 0xffffffffu &&
              Arrays::InsertUnique(state, 6, 9, runtime) == 6u &&
              !Arrays::Contains(state, -1, 9, runtime) &&
              !Arrays::Contains(state, 6, 9, runtime) &&
              !Arrays::Contains(state, 0, 9, runtime) &&
              runtime.reads.empty() && runtime.writes.empty() &&
              runtime.allocations.empty(),
          "invalid indices and empty slot never access element memory");
}

void TestSignedSearchAndDuplicate() {
    State state;
    state.slots[5] = {0xaabbccddu, 0x3000u, 0x300cu, 0x3014u};
    Runtime runtime;
    runtime.words = {{0x3000u, static_cast<std::uint32_t>(-3)},
                     {0x3004u, 0u}, {0x3008u, 9u}};
    for (int value : {-3, 0, 9})
        Check(Arrays::Contains(state, 5, value, runtime),
              "first, middle and last signed values are present");
    for (int value : {-4, -2, 1, 10})
        Check(!Arrays::Contains(state, 5, value, runtime),
              "signed binary search rejects gaps and both ends");
    runtime.reads.clear();
    Check(Arrays::InsertUnique(state, 5, 0, runtime) == 0u &&
              state.slots[5].reserved == 0xaabbccddu &&
              state.slots[5].end == 0x300cu &&
              runtime.writes.empty() && runtime.allocations.empty(),
          "duplicate returns final binary-search midpoint without writing");
}

void TestSpareCapacityAtEndAndMiddle() {
    State state;
    state.slots[2] = {0x12345678u, 0x2000u, 0x2008u, 0x2010u};
    Runtime runtime;
    runtime.words = {{0x2000u, static_cast<std::uint32_t>(-5)},
                     {0x2004u, 8u}};
    Check(Arrays::InsertUnique(state, 2, 12, runtime) == 0x2008u &&
              state.slots[2].end == 0x200cu &&
              state.slots[2].capacity_end == 0x2010u &&
              state.slots[2].reserved == 0x12345678u &&
              runtime.words[0x2008u] == 12u &&
              runtime.allocations.empty() && runtime.frees.empty(),
          "tail append returns old end and leaves spare capacity");

    state.slots[1] = {0u, 0x1000u, 0x100cu, 0x1018u};
    runtime.words[0x1000u] = 1u;
    runtime.words[0x1004u] = 5u;
    runtime.words[0x1008u] = 9u;
    Check(Arrays::InsertUnique(state, 1, 7, runtime) == 0x100cu &&
              state.slots[1].end == 0x1010u &&
              runtime.words[0x1000u] == 1u &&
              runtime.words[0x1004u] == 5u &&
              runtime.words[0x1008u] == 7u &&
              runtime.words[0x100cu] == 9u,
          "middle insertion shifts one suffix entry and returns position plus four");
    Check(Arrays::InsertUnique(state, 1, 0, runtime) == 0x1004u &&
              state.slots[1].end == 0x1014u &&
              runtime.words[0x1000u] == 0u &&
              runtime.words[0x1004u] == 1u &&
              runtime.words[0x1008u] == 5u &&
              runtime.words[0x100cu] == 7u &&
              runtime.words[0x1010u] == 9u,
          "head insertion shifts the full suffix without allocation");
}

void TestEmptyAndFullGrowth() {
    State state;
    Runtime runtime;
    Check(Arrays::InsertUnique(state, 0, 3, runtime) == 0x4004u &&
              state.slots[0].begin == 0x4000u &&
              state.slots[0].end == 0x4004u &&
              state.slots[0].capacity_end == 0x4004u &&
              runtime.allocations == std::vector<std::uint32_t>{4u} &&
              runtime.frees == std::vector<std::uint32_t>{0u} &&
              runtime.words[0x4000u] == 3u,
          "empty slot allocates one DWORD and returns new end");

    runtime.next_allocation = 0x5000u;
    runtime.allocations.clear();
    runtime.frees.clear();
    Check(Arrays::InsertUnique(state, 0, 1, runtime) == 0x5008u &&
              state.slots[0].begin == 0x5000u &&
              state.slots[0].end == 0x5008u &&
              state.slots[0].capacity_end == 0x5008u &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.frees == std::vector<std::uint32_t>{0x4000u} &&
              runtime.words[0x5000u] == 1u &&
              runtime.words[0x5004u] == 3u,
          "one-entry growth inserts before the existing value");

    runtime.next_allocation = 0x6000u;
    runtime.allocations.clear();
    runtime.frees.clear();
    Check(Arrays::InsertUnique(state, 0, 2, runtime) == 0x600cu &&
              state.slots[0].begin == 0x6000u &&
              state.slots[0].end == 0x600cu &&
              state.slots[0].capacity_end == 0x6010u &&
              runtime.allocations == std::vector<std::uint32_t>{16u} &&
              runtime.frees == std::vector<std::uint32_t>{0x5000u} &&
              runtime.words[0x6000u] == 1u &&
              runtime.words[0x6004u] == 2u &&
              runtime.words[0x6008u] == 3u,
          "two-entry growth doubles capacity and copies prefix and suffix");
    Check(Arrays::Contains(state, 0, 2, runtime),
          "membership reads the newly allocated sorted range");
}

void TestIndexedIteratorCreation() {
    State state;
    state.slots[0].begin = 0x1000u;
    state.slots[5].begin = 0x5000u;
    IteratorRuntime runtime;
    for (int index : {0, 5}) {
        runtime = {};
        const auto expected_begin = index == 0 ? 0x1000u : 0x5000u;
        Check(Arrays::GetIteratorByIndex(
                  state, 0x3000u, 0xa000u, index, runtime) == 0xa000u &&
                  runtime.allocated_bytes == 12u &&
                  runtime.stored_address == 0x9000u &&
                  runtime.valid.vtable == Arrays::kValidIteratorVtable &&
                  runtime.valid.current == expected_begin &&
                  runtime.valid.owner == 0x3000u +
                      static_cast<std::uint32_t>(index) * 16u &&
                  runtime.output_address == 0xa000u &&
                  runtime.output_value == 0x9000u &&
                  runtime.calls == std::vector<int>{1, 2, 4},
              "valid first and last indices create 12-byte slot iterators");
    }
    for (int index : {-1, 6}) {
        runtime = {};
        Check(Arrays::GetIteratorByIndex(
                  state, 0x3000u, 0xa000u, index, runtime) == 0xa000u &&
                  runtime.allocated_bytes == 8u &&
                  runtime.invalid.vtable == Arrays::kInvalidIteratorVtable &&
                  runtime.invalid.index == 0xffffffffu &&
                  runtime.output_value == 0x9000u &&
                  runtime.calls == std::vector<int>{1, 3, 4},
              "invalid indices create the eight-byte sentinel iterator");
    }
    runtime = {};
    runtime.next_allocation = 0u;
    Check(Arrays::GetIteratorByIndex(
              state, 0x3000u, 0xa000u, 2, runtime) == 0xa000u &&
              runtime.calls == std::vector<int>{1, 4} &&
              runtime.output_value == 0u,
          "valid iterator allocation failure writes null output");
    runtime = {};
    runtime.next_allocation = 0u;
    Check(Arrays::GetIteratorByIndex(
              state, 0x3000u, 0xa000u, 6, runtime) == 0xa000u &&
              runtime.calls == std::vector<int>{1, 4} &&
              runtime.output_value == 0u,
          "invalid iterator allocation failure writes null output");
    pal4::IndexedSortedIteratorRuntimeStub stub;
    Check(Arrays::GetIteratorByIndex(
              state, 0x3000u, 0xa000u, 0, stub) == 0xa000u,
          "default indexed iterator runtime links");
}

void TestFixedOffsetSortedArray() {
    pal4::FixedSortedValueArrayState state;
    state.prefix[0] = 0x12345678u;
    state.values = {0x55u, 0x7000u, 0x7008u, 0x7010u};
    Runtime runtime;
    runtime.words = {{0x7000u, static_cast<std::uint32_t>(-2)},
                     {0x7004u, 8u}};
    Check(pal4::FixedSortedValueArray::Contains(state, -2, runtime) &&
              pal4::FixedSortedValueArray::Contains(state, 8, runtime) &&
              !pal4::FixedSortedValueArray::Contains(state, 0, runtime) &&
              !pal4::FixedSortedValueArray::Contains(state, 9, runtime),
          "fixed +0xa0 array searches signed DWORDs with exact equality");
    pal4::FixedSortedValueArray::InsertUnique(state, 0, runtime);
    Check(state.values.end == 0x700cu &&
              runtime.words[0x7000u] == static_cast<std::uint32_t>(-2) &&
              runtime.words[0x7004u] == 0u &&
              runtime.words[0x7008u] == 8u,
          "fixed array inserts into spare capacity in signed order");
    runtime.writes.clear();
    pal4::FixedSortedValueArray::InsertUnique(state, 8, runtime);
    Check(state.values.end == 0x700cu && runtime.writes.empty(),
          "fixed array skips duplicate values");
    pal4::FixedSortedValueArray::InsertUnique(state, 9, runtime);
    Check(state.values.end == 0x7010u && runtime.words[0x700cu] == 9u,
          "fixed array uses the last spare slot");
    pal4::FixedSortedValueArray::InsertUnique(state, -5, runtime);
    Check(state.prefix[0] == 0x12345678u && state.values.reserved == 0x55u &&
              state.values.begin == 0x4000u &&
              state.values.end == 0x4014u &&
              state.values.capacity_end == 0x4020u &&
              runtime.allocations == std::vector<std::uint32_t>{32u} &&
              runtime.frees == std::vector<std::uint32_t>{0x7000u} &&
              runtime.words[0x4000u] == static_cast<std::uint32_t>(-5) &&
              runtime.words[0x4004u] == static_cast<std::uint32_t>(-2) &&
              runtime.words[0x4008u] == 0u &&
              runtime.words[0x400cu] == 8u &&
              runtime.words[0x4010u] == 9u &&
              pal4::FixedSortedValueArray::Contains(state, -5, runtime),
          "fixed array growth doubles four entries and preserves object prefix");
}

void TestFixedArrayIteratorAndClear() {
    pal4::FixedSortedValueArrayState state;
    state.values = {0x77u, 0x7000u, 0x700cu, 0x7010u};
    IteratorRuntime runtime;
    Check(pal4::FixedSortedValueArray::GetIterator(
              state, 0x3000u, 0xa000u, runtime) == 0xa000u &&
              runtime.allocated_bytes == 12u &&
              runtime.valid.vtable ==
                  Arrays::kValidIteratorVtable &&
              runtime.valid.current == 0x7000u &&
              runtime.valid.owner == 0x30a0u &&
              runtime.output_value == 0x9000u &&
              runtime.calls == std::vector<int>{1, 2, 4},
          "fixed-array iterator holds begin and object +0xa0");
    runtime = {};
    runtime.next_allocation = 0u;
    Check(pal4::FixedSortedValueArray::GetIterator(
              state, 0x3000u, 0xa000u, runtime) == 0xa000u &&
              runtime.output_value == 0u &&
              runtime.calls == std::vector<int>{1, 4},
          "fixed-array iterator allocation failure writes null");
    Check(pal4::FixedSortedValueArray::Clear(state) == 0x700cu &&
              state.values.reserved == 0x77u &&
              state.values.begin == 0x7000u &&
              state.values.end == 0x7000u &&
              state.values.capacity_end == 0x7010u,
          "clear returns old end and resets only the end pointer");
    Check(pal4::FixedSortedValueArray::Clear(state) == 0x7000u,
          "clearing an already empty array returns its begin pointer");
}

void TestFixedArrayProcessing() {
    pal4::FixedSortedValueArrayState state;
    state.values = {0u, 0x7000u, 0x7000u, 0x700cu};
    ProcessRuntime runtime;
    Check(pal4::FixedSortedValueArray::ProcessValues(
              state, 0x3000u, runtime) == 0x7000u &&
              runtime.calls.empty(),
          "empty fixed array returns end without callbacks");
    state.values.end = 0x700cu;
    runtime.words = {{0x7000u, 1u}, {0x7004u, 2u}, {0x7008u, 3u}};
    Check(pal4::FixedSortedValueArray::ProcessValues(
              state, 0x3000u, runtime) == 0x700cu &&
              runtime.calls == std::vector<std::string>{
                  "read:28672", "process:12288:1",
                  "read:28676", "process:12288:2",
                  "read:28680", "process:12288:3"},
          "fixed array processes all elements in order");
    runtime.calls.clear();
    runtime.state = &state;
    runtime.shorten_after_first = true;
    Check(pal4::FixedSortedValueArray::ProcessValues(
              state, 0x3000u, runtime) == 0x7004u &&
              runtime.calls == std::vector<std::string>{
                  "read:28672", "process:12288:1"},
          "fixed array reloads end after callback changes the range");
    pal4::SortedValueProcessRuntimeStub stub;
    pal4::FixedSortedValueArrayState empty;
    Check(pal4::FixedSortedValueArray::ProcessValues(
              empty, 0u, stub) == 0u,
          "default sorted-value process runtime links");
}

}  // namespace

int main() {
    TestIndexGatesAndEmptySlot();
    TestSignedSearchAndDuplicate();
    TestSpareCapacityAtEndAndMiddle();
    TestEmptyAndFullGrowth();
    TestIndexedIteratorCreation();
    TestFixedOffsetSortedArray();
    TestFixedArrayIteratorAndClear();
    TestFixedArrayProcessing();
    State state;
    pal4::IndexedSortedValueArrayRuntimeStub stub;
    Check(!Arrays::Contains(state, 0, 0, stub),
          "default sorted-array runtime links");
    std::cout << "9 indexed sorted value array cases passed\n";
}
