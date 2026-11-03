#include "pal4/dword_sort.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace {

std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Less(std::uint32_t left, std::uint32_t right) {
    comparisons.emplace_back(left, right);
    return left < right;
}

bool Greater(std::uint32_t left, std::uint32_t right) {
    return left > right;
}

bool HighByteLess(std::uint32_t left, std::uint32_t right) {
    return (left >> 24u) < (right >> 24u);
}

void CheckSmallPartition() {
    for (std::size_t size = 0; size <= 16; ++size) {
        std::vector<std::uint32_t> values(size + 1u, 0xfeedfaceu);
        std::iota(values.begin(), values.begin() + size, 0u);
        std::reverse(values.begin(), values.begin() + size);
        const auto original = values;
        comparisons.clear();
        Check(pal4::DwordSort::PartitionLarge(
                  values.data(), values.data() + size, Less) ==
                  static_cast<std::int32_t>(size),
              "small partition returns its untouched element count");
        Check(values == original && comparisons.empty(),
              "at most 16 elements have no quick-sort side effects");
    }
}

void CheckMedianAndPartitions() {
    std::vector<std::uint32_t> values(17u);
    std::iota(values.begin(), values.end(), 1u);
    comparisons.clear();
    const auto remaining = pal4::DwordSort::PartitionLarge(
        values.data(), values.data() + values.size(), Less);
    Check(comparisons.size() >= 2u &&
              comparisons[0] == std::pair{1u, 9u} &&
              comparisons[1] == std::pair{9u, 17u},
          "median-of-three compares first, middle, last in IDA order");
    Check(remaining >= 0 && remaining <= 16,
          "quick partition stops at the 16-element threshold");
    Check(std::is_sorted(values.begin(), values.end()),
          "ascending partition keeps ordered input intact");
}

void CheckInsertion() {
    std::vector<std::uint32_t> empty;
    Check(pal4::DwordSort::InsertionSort(
              empty.data(), empty.data(), Less) == empty.data(),
          "empty insertion returns end");
    std::vector<std::uint32_t> single{42u};
    comparisons.clear();
    Check(pal4::DwordSort::InsertionSort(
              single.data(), single.data() + single.size(), Less) ==
              single.data() + single.size() && comparisons.empty(),
          "single insertion returns end without comparison");

    std::vector<std::uint32_t> values{4u, 7u, 6u, 1u, 3u, 2u, 5u};
    auto* end = values.data() + values.size();
    Check(pal4::DwordSort::InsertionSort(values.data(), end, Less) == end &&
              values == std::vector<std::uint32_t>{1u, 2u, 3u, 4u, 5u, 6u, 7u},
          "insertion handles front relocation and sentinel-based shifts");

    std::vector<std::uint32_t> tagged{
        0x02000001u, 0x01000002u, 0x02000003u, 0x01000004u};
    (void)pal4::DwordSort::InsertionSort(
        tagged.data(), tagged.data() + tagged.size(), HighByteLess);
    Check(tagged == std::vector<std::uint32_t>{
                0x01000002u, 0x01000004u, 0x02000001u, 0x02000003u},
          "equal comparator keys retain their insertion order");
}

void CheckWholeSort() {
    std::mt19937 generator(0x4422a0u);
    for (std::size_t size = 0; size <= 128; ++size) {
        for (int case_index = 0; case_index < 5; ++case_index) {
            std::vector<std::uint32_t> values(size + 1u, 0xfeedfaceu);
            for (std::size_t index = 0; index < size; ++index) {
                values[index] = generator() % 29u;
            }
            auto expected = values;
            std::sort(expected.begin(), expected.begin() + size);
            auto* end = values.data() + size;
            const auto remaining = pal4::DwordSort::PartitionLarge(
                values.data(), end, Less);
            Check(remaining >= 0 && remaining <= 16,
                  "recursive partition leaves a small final range");
            Check(pal4::DwordSort::InsertionSort(
                      values.data(), end, Less) == end && values == expected,
                  "quick partition plus insertion orders duplicate-rich data");
        }
    }

    std::vector<std::uint32_t> descending(65u);
    std::iota(descending.begin(), descending.end(), 0u);
    (void)pal4::DwordSort::PartitionLarge(
        descending.data(), descending.data() + descending.size(), Greater);
    (void)pal4::DwordSort::InsertionSort(
        descending.data(), descending.data() + descending.size(), Greater);
    Check(std::is_sorted(descending.begin(), descending.end(), Greater),
          "caller-defined reverse ordering is respected");
}

void CheckHybridSort() {
    std::mt19937 generator(0x4774a0u);
    for (std::size_t size = 0; size <= 256; ++size) {
        for (int case_index = 0; case_index < 12; ++case_index) {
            std::vector<std::uint32_t> values(size + 1u, 0xfeedfaceu);
            for (std::size_t index = 0; index < size; ++index) {
                values[index] = generator() % 37u;
            }
            auto expected = values;
            std::sort(expected.begin(), expected.begin() + size);
            auto* end = values.data() + size;
            const auto result = pal4::DwordSort::Sort(values.data(), end, Less);
            const auto expected_result = size < 2u
                ? static_cast<std::uintptr_t>(size)
                : reinterpret_cast<std::uintptr_t>(end);
            Check(result == expected_result && values == expected,
                  "hybrid sort orders duplicate-rich ranges and preserves EAX");
        }
    }

    std::vector<std::uint32_t> descending(65u);
    std::iota(descending.begin(), descending.end(), 0u);
    (void)pal4::DwordSort::Sort(
        descending.data(), descending.data() + descending.size(), Greater);
    Check(std::is_sorted(descending.begin(), descending.end(), Greater),
          "hybrid sort respects reverse comparison");

    std::vector<std::uint32_t> boundary{2u, 1u};
    comparisons.clear();
    (void)pal4::DwordSort::Sort(boundary.data(), boundary.data() + 2, Less);
    Check(!comparisons.empty() && comparisons.front() == std::pair{1u, 2u},
          "short range begins with the IDA first-element comparison");
}

}  // namespace

int main() {
    CheckSmallPartition();
    CheckMedianAndPartitions();
    CheckInsertion();
    CheckWholeSort();
    CheckHybridSort();
}
