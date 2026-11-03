#include "pal4/singleton_collection_sort.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ISingletonCollectionSortRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t size) override {
        for (std::uint32_t index = 0u; index < size; ++index) {
            bytes[destination + index] = bytes.at(source + index);
        }
    }
    void Free(std::uint32_t) override { ++free_calls; }
    void ThrowRange() override { throw std::out_of_range("range"); }
    void DetachAndReset(std::uint32_t) override { ++detach_calls; }
    bool AllocateText(std::uint32_t, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(length == 0u && mode == 1u,
              "sorting test records have empty text and use mode one");
        ++allocation_calls;
        return false;
    }
    void SetTextSize(std::uint32_t, std::uint32_t) override {
        Check(false, "failed allocation must not set text size");
    }
    bool Compare(std::uint32_t left, std::uint32_t right) override {
        ++comparison_calls;
        comparison_pairs.emplace_back(left, right);
        const auto left_id = static_cast<std::int32_t>(
            words.at(left + 16u));
        const auto right_id = static_cast<std::int32_t>(
            words.at(right + 16u));
        return left_id > right_id;
    }
    std::uint32_t AllocateTemporary() override {
        const std::uint32_t address = next_temporary;
        next_temporary += 28u;
        ++temporary_calls;
        return address;
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        ++swap_text_calls;
        for (const std::uint32_t offset : {4u, 8u, 12u}) {
            words[destination + offset] = words.at(source + offset);
        }
    }
    std::uint32_t ReleaseText(std::uint32_t record) override {
        released.push_back(record);
        return release_result;
    }

    void Seed(std::uint32_t address, std::uint32_t id,
              std::uint8_t tag = 0u) {
        bytes[address] = tag;
        words[address + 4u] = 0u;
        words[address + 8u] = 0u;
        words[address + 12u] = 0u;
        words[address + 16u] = id;
        words[address + 20u] = id + 100u;
        words[address + 24u] = id + 200u;
    }
    std::vector<std::uint32_t> Ids(std::uint32_t begin,
                                    std::uint32_t count) const {
        std::vector<std::uint32_t> result;
        for (std::uint32_t index = 0u; index < count; ++index) {
            result.push_back(words.at(begin + index * 28u + 16u));
        }
        return result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> released;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparison_pairs;
    std::uint32_t next_temporary = 0x9000u;
    std::uint32_t temporary_calls = 0u;
    std::uint32_t comparison_calls = 0u;
    std::uint32_t swap_text_calls = 0u;
    std::uint32_t allocation_calls = 0u;
    std::uint32_t free_calls = 0u;
    std::uint32_t detach_calls = 0u;
    std::uint32_t release_result = 0u;
};

void CheckSmallRangeStopsImmediately() {
    Runtime runtime;
    const std::uint32_t begin = 0x1000u;
    pal4::SingletonCollectionSort::PartitionLarge(
        begin, begin + 16u * 28u, runtime);
    Check(runtime.comparison_calls == 0u &&
              runtime.temporary_calls == 0u,
          "partition leaves ranges of sixteen or fewer untouched");
    pal4::SingletonCollectionSort::PartitionLarge(begin, begin, runtime);
    Check(runtime.comparison_calls == 0u && runtime.temporary_calls == 0u,
          "empty range does not access records");
}

void CheckMedianPermutations() {
    std::array<std::uint32_t, 3> values{1u, 2u, 3u};
    do {
        Runtime runtime;
        runtime.Seed(0x1000u, values[0]);
        runtime.Seed(0x101cu, values[1]);
        runtime.Seed(0x1038u, values[2]);
        pal4::SingletonCollectionSort::SelectMedian(
            0x1000u, 0x101cu, 0x1038u, 0x2000u, runtime);
        Check(runtime.words[0x2010u] == 2u &&
                  runtime.released == std::vector<std::uint32_t>{
                      0x1000u, 0x101cu, 0x1038u} &&
                  runtime.comparison_calls >= 2u &&
                  runtime.comparison_calls <= 3u,
              "each input ordering selects median and releases three candidates");
    } while (std::next_permutation(values.begin(), values.end()));
}

void CheckMedianEqualKeys() {
    for (const std::array<std::uint32_t, 3> ids : {
             std::array<std::uint32_t, 3>{2u, 2u, 2u},
             std::array<std::uint32_t, 3>{2u, 1u, 1u}}) {
        Runtime runtime;
        constexpr std::array<std::uint32_t, 3> addresses{
            0x1000u, 0x101cu, 0x1038u};
        for (std::uint32_t index = 0u; index < addresses.size(); ++index) {
            runtime.Seed(addresses[index], ids[index]);
            runtime.words[addresses[index] + 20u] = 100u + index;
        }
        pal4::SingletonCollectionSort::SelectMedian(
            addresses[0], addresses[1], addresses[2], 0x2000u, runtime);
        const std::uint32_t selected_metadata =
            ids[0] == ids[1] ? 101u : 102u;
        Check(runtime.words.at(0x2000u + 20u) == selected_metadata &&
                  runtime.released == std::vector<std::uint32_t>(
                                          addresses.begin(), addresses.end()),
              "median tie branch preserves selected record and releases all inputs");
    }
}

void CheckAlreadyPartitionedRange() {
    Runtime runtime;
    const std::uint32_t begin = 0x1000u;
    for (std::uint32_t index = 0u; index < 17u; ++index) {
        runtime.Seed(begin + index * 28u, 17u - index,
                     static_cast<std::uint8_t>(index));
    }
    pal4::SingletonCollectionSort::PartitionLarge(
        begin, begin + 17u * 28u, runtime);
    Check(runtime.temporary_calls == 4u &&
              runtime.swap_text_calls == 0u &&
              runtime.Ids(begin, 17u).front() == 17u &&
              runtime.Ids(begin, 17u).back() == 1u,
          "seventeen descending IDs partition once without a swap");
}

void CheckSwapsAndRecursiveShrink() {
    Runtime runtime;
    const std::uint32_t begin = 0x1000u;
    constexpr std::uint32_t count = 50u;
    for (std::uint32_t index = 0u; index < count; ++index) {
        runtime.Seed(begin + index * 28u, index + 1u,
                     static_cast<std::uint8_t>(index));
    }
    pal4::SingletonCollectionSort::PartitionLarge(
        begin, begin + count * 28u, runtime);
    auto actual = runtime.Ids(begin, count);
    std::sort(actual.begin(), actual.end());
    Check(runtime.temporary_calls > 4u &&
              runtime.swap_text_calls > 0u &&
              runtime.released.size() >= runtime.temporary_calls &&
              actual.front() == 1u && actual.back() == count,
          "large reversed range swaps records and recursively shrinks partitions");
    for (std::uint32_t index = 0u; index < count; ++index) {
        Check(actual[index] == index + 1u &&
                  runtime.bytes.at(begin + index * 28u) ==
                      static_cast<std::uint8_t>(index),
              "swaps preserve ID permutation and leave byte-zero tags in place");
    }
}

void CheckInsertionShifts() {
    Runtime runtime;
    const std::uint32_t begin = 0x1000u;
    runtime.Seed(begin, 9u, 1u);
    runtime.Seed(begin + 28u, 7u, 2u);
    runtime.Seed(begin + 56u, 5u, 3u);
    runtime.Seed(begin + 84u, 0u, 4u);
    runtime.Seed(0x9000u, 8u, 0u);
    pal4::SingletonCollectionSort::InsertSortedElement(
        begin + 84u, 0x9000u, runtime);
    Check(runtime.Ids(begin, 4u) ==
                  std::vector<std::uint32_t>{9u, 8u, 7u, 5u} &&
              runtime.comparison_calls == 3u &&
              runtime.comparison_pairs ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x9000u, begin + 56u}, {0x9000u, begin + 28u},
                      {0x9000u, begin}} &&
              runtime.released == std::vector<std::uint32_t>{0x9000u},
          "insertion shifts two records and releases its temporary");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        Check(runtime.bytes.at(begin + index * 28u) == index + 1u,
              "insertion keeps each destination prefix byte in place");
    }

    Runtime no_shift;
    no_shift.Seed(begin, 9u);
    no_shift.Seed(begin + 28u, 7u);
    no_shift.Seed(begin + 56u, 5u);
    no_shift.Seed(begin + 84u, 0u);
    no_shift.Seed(0x9000u, 4u);
    pal4::SingletonCollectionSort::InsertSortedElement(
        begin + 84u, 0x9000u, no_shift);
    Check(no_shift.Ids(begin, 4u) ==
                  std::vector<std::uint32_t>{9u, 7u, 5u, 4u} &&
              no_shift.comparison_calls == 1u &&
              no_shift.released == std::vector<std::uint32_t>{0x9000u},
          "already-positioned temporary does not shift any predecessor");
}

void CheckInsertionSortAndCombinedPipeline() {
    Runtime short_range;
    pal4::SingletonCollectionSort::InsertionSort(
        0x1000u, 0x1000u, short_range);
    pal4::SingletonCollectionSort::InsertionSort(
        0x1000u, 0x101cu, short_range);
    Check(short_range.temporary_calls == 0u,
          "zero and one-record ranges construct no temporaries");

    Runtime three;
    const std::uint32_t begin = 0x1000u;
    three.Seed(begin, 1u, 1u);
    three.Seed(begin + 28u, 3u, 2u);
    three.Seed(begin + 56u, 2u, 3u);
    pal4::SingletonCollectionSort::InsertionSort(
        begin, begin + 84u, three);
    Check(three.Ids(begin, 3u) ==
                  std::vector<std::uint32_t>{3u, 2u, 1u} &&
              three.swap_text_calls > 0u,
          "front insertion followed by internal insertion orders three IDs");

    Runtime combined;
    constexpr std::uint32_t count = 50u;
    for (std::uint32_t index = 0u; index < count; ++index) {
        combined.Seed(begin + index * 28u, index + 1u,
                      static_cast<std::uint8_t>(index));
    }
    pal4::SingletonCollectionSort::PartitionLarge(
        begin, begin + count * 28u, combined);
    pal4::SingletonCollectionSort::InsertionSort(
        begin, begin + count * 28u, combined);
    for (std::uint32_t index = 0u; index < count; ++index) {
        Check(combined.words.at(begin + index * 28u + 16u) ==
                  count - index &&
                  combined.bytes.at(begin + index * 28u) ==
                      static_cast<std::uint8_t>(index),
              "partition plus insertion produces descending IDs without moving tags");
    }
}

void CheckInsertionSortPreservesEqualKeyOrder() {
    Runtime runtime;
    constexpr std::uint32_t begin = 0x2000u;
    constexpr std::array<std::uint32_t, 5> ids{4u, 2u, 4u, 3u, 2u};
    for (std::uint32_t index = 0u; index < ids.size(); ++index) {
        const std::uint32_t record = begin + index * 28u;
        runtime.Seed(record, ids[index], static_cast<std::uint8_t>(index));
        runtime.words[record + 20u] = 100u + index;
    }
    pal4::SingletonCollectionSort::InsertionSort(
        begin, begin + static_cast<std::uint32_t>(ids.size()) * 28u,
        runtime);
    Check(runtime.Ids(begin, 5u) ==
              std::vector<std::uint32_t>{4u, 4u, 3u, 2u, 2u},
          "insertion sorts repeated keys in comparator order");
    constexpr std::array<std::uint32_t, 5> order{100u, 102u, 103u,
                                                   101u, 104u};
    for (std::uint32_t index = 0u; index < order.size(); ++index) {
        Check(runtime.words.at(begin + index * 28u + 20u) == order[index],
              "equal keys preserve original metadata order");
    }
}

void CheckSwapCollectionElements() {
    Runtime runtime;
    runtime.Seed(0x1000u, 7u, 0x11u);
    runtime.Seed(0x101cu, 3u, 0x22u);
    runtime.words[0x1004u] = 0x3000u;
    runtime.words[0x101cu + 4u] = 0x4000u;
    pal4::SingletonCollectionSort::SwapCollectionElements(
        0x1000u, 0x101cu, runtime);
    Check(runtime.Ids(0x1000u, 2u) ==
                  std::vector<std::uint32_t>{3u, 7u} &&
              runtime.words.at(0x1000u + 20u) == 103u &&
              runtime.words.at(0x1000u + 24u) == 203u &&
              runtime.words.at(0x101cu + 20u) == 107u &&
              runtime.words.at(0x101cu + 24u) == 207u &&
              runtime.words.at(0x1004u) == 0x4000u &&
              runtime.words.at(0x101cu + 4u) == 0x3000u &&
              runtime.bytes.at(0x1000u) == 0x11u &&
              runtime.bytes.at(0x101cu) == 0x22u &&
              runtime.bytes.at(0x9000u) == 0x11u &&
              runtime.swap_text_calls == 3u &&
              runtime.released == std::vector<std::uint32_t>{0x9000u},
          "swap preserves prefix bytes and releases the temporary text");

    Runtime self;
    self.Seed(0x2000u, 9u, 0x44u);
    pal4::SingletonCollectionSort::SwapCollectionElements(
        0x2000u, 0x2000u, self);
    Check(self.Ids(0x2000u, 1u) == std::vector<std::uint32_t>{9u} &&
              self.swap_text_calls == 3u && self.released.size() == 1u,
          "self swap still performs three text assignments and cleanup");
}

}  // namespace

int main() {
    CheckSmallRangeStopsImmediately();
    CheckMedianPermutations();
    CheckMedianEqualKeys();
    CheckAlreadyPartitionedRange();
    CheckSwapsAndRecursiveShrink();
    CheckInsertionShifts();
    CheckInsertionSortAndCombinedPipeline();
    CheckInsertionSortPreservesEqualKeyOrder();
    CheckSwapCollectionElements();
    std::cout << "9 singleton collection sort test cases passed\n";
}
