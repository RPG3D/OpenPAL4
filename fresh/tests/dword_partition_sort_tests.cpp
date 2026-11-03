#include "pal4/dword_partition_sort.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDwordPartitionSortRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;
    std::uint32_t writes = 0;
    bool descending = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        ++writes;
    }
    bool Less(std::uint32_t left, std::uint32_t right) override {
        comparisons.emplace_back(left, right);
        if (descending)
            return static_cast<std::int32_t>(left) >
                   static_cast<std::int32_t>(right);
        return static_cast<std::int32_t>(left) <
               static_cast<std::int32_t>(right);
    }
};

Runtime WithWords(const std::vector<std::int32_t>& values) {
    Runtime runtime;
    for (std::size_t i = 0; i < values.size(); ++i)
        runtime.words[0x2000 + static_cast<std::uint32_t>(i) * 4] =
            static_cast<std::uint32_t>(values[i]);
    return runtime;
}

std::vector<std::int32_t> ReadValues(Runtime& runtime, std::size_t count) {
    std::vector<std::int32_t> values;
    for (std::size_t i = 0; i < count; ++i)
        values.push_back(static_cast<std::int32_t>(runtime.ReadWord(
            0x2000 + static_cast<std::uint32_t>(i) * 4)));
    return values;
}

void CheckPartition() {
    const std::vector<std::int32_t> original{5, -2, 9, 1, 7, 3, 5, -9};
    auto runtime = WithWords(original);
    const auto middle = pal4::DwordPartitionSort::Partition(
        0x2000, 0x2020, 5, runtime);
    Check(middle > 0x2000 && middle < 0x2020 && runtime.writes > 0,
          "partition moves a nontrivial split and swaps values");
    auto values = ReadValues(runtime, original.size());
    auto sorted = values;
    std::sort(sorted.begin(), sorted.end());
    auto original_sorted = original;
    std::sort(original_sorted.begin(), original_sorted.end());
    Check(sorted == original_sorted, "partition preserves all DWORD values");
    const auto left_count = (middle - 0x2000) / 4;
    for (std::size_t i = 0; i < values.size(); ++i)
        Check(i < left_count ? values[i] <= 5 : values[i] >= 5,
              "partition separates values around its signed pivot");

    auto descending = WithWords(original);
    descending.descending = true;
    const auto descending_middle = pal4::DwordPartitionSort::Partition(
        0x2000, 0x2020, 5, descending);
    values = ReadValues(descending, original.size());
    const auto descending_left = (descending_middle - 0x2000) / 4;
    for (std::size_t i = 0; i < values.size(); ++i)
        Check(i < descending_left ? values[i] >= 5 : values[i] <= 5,
              "partition honors the injected descending comparator");
}

void CheckLargeSorter() {
    std::vector<std::int32_t> short_values;
    for (std::int32_t value = 16; value > 0; --value)
        short_values.push_back(value);
    auto short_runtime = WithWords(short_values);
    Check(pal4::DwordPartitionSort::SortLarge(
              0x2000, 0x2040, short_runtime) == 16 &&
              short_runtime.writes == 0 &&
              ReadValues(short_runtime, 16) == short_values,
          "ranges of sixteen or fewer remain for the caller's insertion pass");

    std::vector<std::int32_t> long_values;
    for (std::int32_t value = 20; value > -20; --value)
        long_values.push_back(value);
    auto long_runtime = WithWords(long_values);
    const auto remaining = pal4::DwordPartitionSort::SortLarge(
        0x2000, 0x20a0, long_runtime);
    Check(remaining <= 16 && long_runtime.writes > 0,
          "large range partitions until the active segment is at most sixteen");
    auto result = ReadValues(long_runtime, 40);
    std::sort(result.begin(), result.end());
    std::sort(long_values.begin(), long_values.end());
    Check(result == long_values,
          "recursive partitioning preserves the entire signed input set");

    std::vector<std::int32_t> equal_values(40, -7);
    auto equal_runtime = WithWords(equal_values);
    Check(pal4::DwordPartitionSort::SortLarge(
              0x2000, 0x20a0, equal_runtime) <= 16 &&
              ReadValues(equal_runtime, 40) == equal_values,
          "equal pivots still terminate and preserve all values");
}

}  // namespace

int main() {
    try {
        CheckPartition();
        CheckLargeSorter();
        std::cout << "dword_partition_sort_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
