#include "pal4/dynamic_record_array24.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::uint32_t kArray = 0x1000u;
constexpr std::uint32_t kSource = 0x5000u;
constexpr std::uint32_t kStride = pal4::DynamicRecordArray24::kStride;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void SetRecord(pal4::DynamicRecordArray24MemoryStub& memory,
               const std::uint32_t address, const std::uint32_t marker) {
    for (std::uint32_t index = 0; index < 6u; ++index)
        memory.WriteDword(address + 4u * index, marker * 100u + index);
}

void CheckRecord(const pal4::DynamicRecordArray24MemoryStub& memory,
                 const std::uint32_t address, const std::uint32_t marker) {
    for (std::uint32_t index = 0; index < 6u; ++index)
        Check(memory.ReadDword(address + 4u * index) == marker * 100u + index,
              "all six record words are copied in order");
}

std::uint32_t Initialize(pal4::DynamicRecordArray24MemoryStub& memory,
                         const std::uint32_t capacity,
                         const std::vector<std::uint32_t>& markers) {
    const auto begin = memory.Allocate(capacity * kStride);
    memory.WriteDword(kArray + 4u, begin);
    memory.WriteDword(kArray + 8u,
                      begin + static_cast<std::uint32_t>(markers.size()) * kStride);
    memory.WriteDword(kArray + 12u, begin + capacity * kStride);
    for (std::uint32_t index = 0u; index < markers.size(); ++index)
        SetRecord(memory, begin + index * kStride, markers[index]);
    return begin;
}

void CheckContents(const pal4::DynamicRecordArray24MemoryStub& memory,
                   const std::vector<std::uint32_t>& markers,
                   const std::uint32_t capacity) {
    const auto begin = memory.ReadDword(kArray + 4u);
    Check(memory.ReadDword(kArray + 8u) == begin + markers.size() * kStride &&
              memory.ReadDword(kArray + 12u) == begin + capacity * kStride,
          "begin, end and capacity pointers describe the expected array");
    for (std::uint32_t index = 0u; index < markers.size(); ++index)
        CheckRecord(memory, begin + index * kStride, markers[index]);
}

void FirstAllocationAndGrowth() {
    pal4::DynamicRecordArray24MemoryStub memory;
    SetRecord(memory, kSource, 9u);
    Check(pal4::DynamicRecordArray24::Insert(kArray, 0u, 3u, kSource,
                                               memory) == 0u,
          "first allocation returns observed zero EAX");
    CheckContents(memory, {9u, 9u, 9u}, 3u);
    Check(memory.FreedBlocks() == std::vector<std::uint32_t>{0u},
          "first allocation still calls free on the null old base");

    const auto old_begin = memory.ReadDword(kArray + 4u);
    SetRecord(memory, kSource, 7u);
    Check(pal4::DynamicRecordArray24::Insert(
              kArray, old_begin + kStride, 2u, kSource, memory) == 0u,
          "growth path returns observed zero EAX");
    CheckContents(memory, {9u, 7u, 7u, 9u, 9u}, 6u);
    Check(memory.FreedBlocks().back() == old_begin &&
              memory.ReadDword(kArray + 4u) != old_begin,
          "growth frees old allocation and replaces its start pointer");
}

void InPlaceWithShortSuffix() {
    pal4::DynamicRecordArray24MemoryStub memory;
    const auto begin = Initialize(memory, 6u, {1u, 2u, 3u});
    SetRecord(memory, kSource, 8u);
    const auto end = begin + 5u * kStride;
    Check(pal4::DynamicRecordArray24::Insert(
              kArray, begin + 2u * kStride, 2u, kSource, memory) == end,
          "short-suffix branch returns new end");
    CheckContents(memory, {1u, 2u, 8u, 8u, 3u}, 6u);
    Check(memory.FreedBlocks().empty(), "in-place branch does not free storage");
}

void InPlaceWithLongSuffix() {
    pal4::DynamicRecordArray24MemoryStub memory;
    const auto begin = Initialize(memory, 7u, {1u, 2u, 3u, 4u});
    SetRecord(memory, kSource, 8u);
    Check(pal4::DynamicRecordArray24::Insert(
              kArray, begin + kStride, 1u, kSource, memory) ==
              begin + 2u * kStride,
          "long-suffix branch returns the end of the inserted range");
    CheckContents(memory, {1u, 8u, 2u, 3u, 4u}, 7u);
    Check(pal4::DynamicRecordArray24::Insert(
              kArray, begin + 2u * kStride, 0u, kSource, memory) == 0u,
          "zero-count branch returns observed zero EAX");
    CheckContents(memory, {1u, 8u, 2u, 3u, 4u}, 7u);
}

void SourceAliasIsReadAfterShifting() {
    pal4::DynamicRecordArray24MemoryStub memory;
    const auto begin = Initialize(memory, 5u, {1u, 2u, 3u});
    Check(pal4::DynamicRecordArray24::Insert(
              kArray, begin, 1u, begin + kStride, memory) ==
              begin + kStride,
          "aliasing insertion returns the inserted-range end");
    CheckContents(memory, {1u, 1u, 2u, 3u}, 5u);
}

void MixedInsertionsAndOverflow() {
    pal4::DynamicRecordArray24MemoryStub memory;
    std::vector<std::uint32_t> expected;
    for (std::uint32_t step = 0u; step < 40u; ++step) {
        const auto begin = memory.ReadDword(kArray + 4u);
        const auto end = memory.ReadDword(kArray + 8u);
        const auto capacity_end = memory.ReadDword(kArray + 12u);
        const auto position_index = expected.empty() ? 0u :
            (step * 7u) % (static_cast<std::uint32_t>(expected.size()) + 1u);
        const auto count = step % 4u;
        const auto marker = step + 10u;
        const auto position = begin + position_index * kStride;
        const auto free_count = (capacity_end - end) / kStride;
        const auto suffix_count =
            static_cast<std::uint32_t>(expected.size()) - position_index;
        const auto expected_return = free_count < count || count == 0u ? 0u :
            (suffix_count < count ? end + count * kStride :
                                    position + count * kStride);
        SetRecord(memory, kSource, marker);
        Check(pal4::DynamicRecordArray24::Insert(
                  kArray, position, count, kSource, memory) == expected_return,
              "mixed operation preserves the path-specific return value");
        expected.insert(expected.begin() + position_index, count, marker);
        const auto current_begin = memory.ReadDword(kArray + 4u);
        const auto current_capacity =
            (memory.ReadDword(kArray + 12u) - current_begin) / kStride;
        CheckContents(memory, expected, current_capacity);
    }

    bool overflow = false;
    try {
        (void)pal4::DynamicRecordArray24::Insert(
            kArray, memory.ReadDword(kArray + 8u),
            std::numeric_limits<std::uint32_t>::max(), kSource, memory);
    } catch (const std::length_error&) {
        overflow = true;
    }
    Check(overflow, "impossible 32-bit insertion size is rejected");
}

}  // namespace

int main() {
    FirstAllocationAndGrowth();
    InPlaceWithShortSuffix();
    InPlaceWithLongSuffix();
    SourceAliasIsReadAfterShifting();
    MixedInsertionsAndOverflow();
    std::cout << "5 dynamic 24-byte record array test groups passed\n";
}
