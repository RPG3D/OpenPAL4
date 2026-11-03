#include "pal4/record_array12.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

using Array = pal4::RecordArray12;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRecordArray12Runtime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_allocation = 0x5000u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocations.push_back(size);
        return next_allocation;
    }
    void Free(const std::uint32_t pointer) override {
        frees.push_back(pointer);
    }
};

void WriteRecord(Runtime& runtime, const std::uint32_t address,
                 const std::array<std::uint32_t, 3> record) {
    for (auto i = 0u; i < record.size(); ++i)
        runtime.words[address + i * 4u] = record[i];
}

std::array<std::uint32_t, 3> ReadRecord(Runtime& runtime,
                                        const std::uint32_t address) {
    return {runtime.words[address], runtime.words[address + 4u],
            runtime.words[address + 8u]};
}

void EmptyAndZeroCount() {
    Runtime runtime;
    constexpr std::uint32_t array = 0x6000u;
    constexpr std::uint32_t source = 0x8000u;
    constexpr std::array<std::uint32_t, 3> record = {1u, 2u, 3u};
    WriteRecord(runtime, source, record);
    Check(Array::Insert(array, 0u, 0u, source, runtime) &&
              runtime.allocations.empty() &&
              !runtime.words.contains(array + 4u),
          "zero count leaves an empty array untouched");
    Check(Array::Insert(array, 0u, 1u, source, runtime) &&
              runtime.allocations == (std::vector<std::uint32_t>{12u}) &&
              runtime.frees == (std::vector<std::uint32_t>{0u}) &&
              runtime.words[array + 4u] == 0x5000u &&
              runtime.words[array + 8u] == 0x500cu &&
              runtime.words[array + 12u] == 0x500cu &&
              ReadRecord(runtime, 0x5000u) == record,
          "empty array allocates one complete three-word record");
}

void SpareCapacity() {
    Runtime runtime;
    constexpr std::uint32_t array = 0x6000u;
    constexpr std::uint32_t source = 0x8000u;
    runtime.words[array + 4u] = 0x5000u;
    runtime.words[array + 8u] = 0x5024u;
    runtime.words[array + 12u] = 0x5060u;
    WriteRecord(runtime, 0x5000u, {10u, 11u, 12u});
    WriteRecord(runtime, 0x500cu, {20u, 21u, 22u});
    WriteRecord(runtime, 0x5018u, {30u, 31u, 32u});
    WriteRecord(runtime, source, {99u, 98u, 97u});
    Check(Array::Insert(array, 0x500cu, 2u, source, runtime) &&
              runtime.words[array + 8u] == 0x503cu &&
              runtime.words[array + 12u] == 0x5060u &&
              ReadRecord(runtime, 0x5000u) ==
                  (std::array<std::uint32_t, 3>{10u, 11u, 12u}) &&
              ReadRecord(runtime, 0x500cu) ==
                  (std::array<std::uint32_t, 3>{99u, 98u, 97u}) &&
              ReadRecord(runtime, 0x5018u) ==
                  (std::array<std::uint32_t, 3>{99u, 98u, 97u}) &&
              ReadRecord(runtime, 0x5024u) ==
                  (std::array<std::uint32_t, 3>{20u, 21u, 22u}) &&
              ReadRecord(runtime, 0x5030u) ==
                  (std::array<std::uint32_t, 3>{30u, 31u, 32u}) &&
              runtime.allocations.empty() && runtime.frees.empty(),
          "spare-capacity middle insertion shifts tail without allocation");

    WriteRecord(runtime, source, {7u, 8u, 9u});
    Check(Array::Insert(array, 0x503cu, 1u, source, runtime) &&
              runtime.words[array + 8u] == 0x5048u &&
              ReadRecord(runtime, 0x503cu) ==
                  (std::array<std::uint32_t, 3>{7u, 8u, 9u}),
          "spare-capacity append writes at old end");

    runtime.words[array + 8u] = 0x5024u;
    WriteRecord(runtime, 0x5018u, {31u, 32u, 33u});
    Check(Array::Insert(array, 0x5018u, 2u, 0x5018u, runtime) &&
              ReadRecord(runtime, 0x5018u) ==
                  (std::array<std::uint32_t, 3>{31u, 32u, 33u}) &&
              ReadRecord(runtime, 0x5024u) ==
                  (std::array<std::uint32_t, 3>{31u, 32u, 33u}) &&
              ReadRecord(runtime, 0x5030u) ==
                  (std::array<std::uint32_t, 3>{31u, 32u, 33u}) &&
              runtime.words[array + 8u] == 0x503cu,
          "in-place insertion snapshots a value inside the shifted tail");
}

void GrowthAndAliasing() {
    Runtime runtime;
    constexpr std::uint32_t array = 0x6000u;
    runtime.words[array + 4u] = 0x4000u;
    runtime.words[array + 8u] = 0x4018u;
    runtime.words[array + 12u] = 0x4018u;
    WriteRecord(runtime, 0x4000u, {1u, 2u, 3u});
    WriteRecord(runtime, 0x400cu, {4u, 5u, 6u});
    Check(Array::Insert(array, 0x400cu, 1u, 0x4000u, runtime) &&
              runtime.allocations == (std::vector<std::uint32_t>{48u}) &&
              runtime.frees == (std::vector<std::uint32_t>{0x4000u}) &&
              runtime.words[array + 4u] == 0x5000u &&
              runtime.words[array + 8u] == 0x5024u &&
              runtime.words[array + 12u] == 0x5030u &&
              ReadRecord(runtime, 0x5000u) ==
                  (std::array<std::uint32_t, 3>{1u, 2u, 3u}) &&
              ReadRecord(runtime, 0x500cu) ==
                  (std::array<std::uint32_t, 3>{1u, 2u, 3u}) &&
              ReadRecord(runtime, 0x5018u) ==
                  (std::array<std::uint32_t, 3>{4u, 5u, 6u}),
          "growth copies prefix, aliased value and suffix before freeing old data");
}

void FailureAndInvalidPosition() {
    Runtime runtime;
    constexpr std::uint32_t array = 0x6000u;
    runtime.words[array + 4u] = 0x4000u;
    runtime.words[array + 8u] = 0x400cu;
    runtime.words[array + 12u] = 0x400cu;
    WriteRecord(runtime, 0x4000u, {1u, 2u, 3u});
    WriteRecord(runtime, 0x8000u, {9u, 8u, 7u});
    runtime.next_allocation = 0u;
    Check(!Array::Insert(array, 0x400cu, 1u, 0x8000u, runtime) &&
              runtime.words[array + 4u] == 0x4000u &&
              runtime.words[array + 8u] == 0x400cu &&
              runtime.words[array + 12u] == 0x400cu &&
              ReadRecord(runtime, 0x4000u) ==
                  (std::array<std::uint32_t, 3>{1u, 2u, 3u}) &&
              runtime.frees.empty(),
          "allocation failure preserves old array contents and ownership");
    runtime.allocations.clear();
    Check(!Array::Insert(array, 0x4004u, 1u, 0x8000u, runtime) &&
              runtime.allocations.empty(),
          "misaligned insertion position fails before allocation");
}

}  // namespace

int main() {
    try {
        EmptyAndZeroCount();
        SpareCapacity();
        GrowthAndAliasing();
        FailureAndInvalidPosition();
        pal4::RecordArray12RuntimeStub stub;
        Check(!Array::Insert(0u, 0u, 1u, 0u, stub),
              "unbound native runtime remains linkable");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
