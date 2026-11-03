#include "pal4/collection_name_sort.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionPointerSortRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> names;
    std::uint32_t first{0x4000u};
    std::uint32_t end{0x4000u};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address >= first && address < end && address % 4u == 0u,
              "sort reads inside the array");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address >= first && address < end && address % 4u == 0u,
              "sort writes inside the array");
        words[address] = value;
    }
    std::int32_t CompareItems(const std::uint32_t first_item,
                              const std::uint32_t second_item) override {
        const auto& first_name = names.at(first_item);
        const auto& second_name = names.at(second_item);
        return first_name < second_name ? -1 :
               first_name > second_name ? 1 : 0;
    }
    std::uint32_t AllocateMemory(std::uint32_t) override { return 0u; }
    void FreeMemory(std::uint32_t) override {}
    void InsertionSort(std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t) override {}
    std::uint32_t CompareThree(std::uint32_t, std::uint32_t,
                                std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t Partition(std::uint32_t, std::uint32_t,
                            std::uint32_t) override { return 0u; }
    void PartitionRange(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
};

std::vector<std::uint32_t> Values(const Runtime& runtime) {
    std::vector<std::uint32_t> values;
    for (auto at = runtime.first; at != runtime.end; at += 4u)
        values.push_back(runtime.words.at(at));
    return values;
}

void TestInsertion() {
    Runtime runtime;
    pal4::CollectionNameSort::InsertionSort(runtime.first, runtime.end,
                                             runtime);
    runtime.end = runtime.first + 4u;
    runtime.words[runtime.first] = 1u;
    runtime.names[1u] = "solo";
    pal4::CollectionNameSort::InsertionSort(runtime.first, runtime.end,
                                             runtime);
    Check(Values(runtime) == std::vector<std::uint32_t>{1u},
          "empty and singleton insertion ranges are unchanged");

    runtime.names = {{1u, "z"}, {2u, "b"}, {3u, "b"}, {4u, "a"}};
    runtime.end = runtime.first + 16u;
    runtime.words = {{0x4000u, 1u}, {0x4004u, 2u},
                     {0x4008u, 3u}, {0x400cu, 4u}};
    pal4::CollectionNameSort::InsertionSort(runtime.first, runtime.end,
                                             runtime);
    Check(Values(runtime) == std::vector<std::uint32_t>{4u, 2u, 3u, 1u},
          "insertion sort orders names and retains equal-name input order");
}

void TestCompareThree() {
    Runtime runtime;
    runtime.names = {{1u, "a"}, {2u, "b"}, {3u, "c"},
                     {4u, "a"}, {5u, "b"}};
    const std::vector<std::vector<std::uint32_t>> orders{
        {1u, 2u, 3u}, {1u, 3u, 2u}, {2u, 1u, 3u},
        {2u, 3u, 1u}, {3u, 1u, 2u}, {3u, 2u, 1u}};
    for (const auto& order : orders)
        Check(pal4::CollectionNameSort::CompareThree(
                  order[0], order[1], order[2], runtime) == 2u,
              "median-of-three selects the middle name in all permutations");
    Check(pal4::CollectionNameSort::CompareThree(
              1u, 4u, 3u, runtime) == 1u &&
              pal4::CollectionNameSort::CompareThree(
                  1u, 2u, 5u, runtime) == 5u &&
              pal4::CollectionNameSort::CompareThree(
                  1u, 4u, 4u, runtime) == 4u,
          "median-of-three follows IDA tie branches");
}

void TestPartition(const std::size_t count, const bool same_names) {
    Runtime runtime;
    runtime.end = runtime.first + static_cast<std::uint32_t>(count * 4u);
    for (std::size_t index = 0; index < count; ++index) {
        const auto item = static_cast<std::uint32_t>(index + 1u);
        runtime.words[runtime.first + static_cast<std::uint32_t>(index * 4u)] =
            item;
        runtime.names[item] = same_names ? "same" :
            std::to_string((count - index + index % 5u) % 23u);
    }
    const auto before = Values(runtime);
    const auto remaining = pal4::CollectionNameSort::PartitionLarge(
        runtime.first, runtime.end, runtime);
    Check(remaining <= 16u, "large partition leaves a short final range");
    auto after = Values(runtime);
    std::sort(after.begin(), after.end());
    Check(after == before, "partition retains every original pointer");
    pal4::CollectionNameSort::InsertionSort(runtime.first, runtime.end,
                                             runtime);
    const auto sorted = Values(runtime);
    Check(std::is_sorted(sorted.begin(), sorted.end(),
                         [&](const auto left, const auto right) {
                             return runtime.names.at(left) <
                                    runtime.names.at(right);
                         }),
          "insertion pass completes partitioned name order");
}

void TestPivotPartition() {
    for (const auto& input : {
             std::vector<std::string>{"f", "a", "d", "c", "e", "b"},
             std::vector<std::string>{"x", "x", "x", "x", "x", "x"}}) {
        Runtime runtime;
        runtime.end = runtime.first +
                      static_cast<std::uint32_t>(input.size() * 4u);
        for (std::size_t index = 0; index < input.size(); ++index) {
            const auto item = static_cast<std::uint32_t>(index + 1u);
            runtime.words[runtime.first + static_cast<std::uint32_t>(index * 4u)] =
                item;
            runtime.names[item] = input[index];
        }
        const auto pivot = 3u;
        const auto split = pal4::CollectionNameSort::Partition(
            runtime.first, runtime.end, pivot, runtime);
        Check(split > runtime.first && split < runtime.end &&
                  split % 4u == 0u,
              "partition returns an interior aligned boundary");
        for (auto address = runtime.first; address != split; address += 4u)
            Check(runtime.CompareItems(runtime.words.at(address), pivot) <= 0,
                  "left partition is no greater than pivot");
        for (auto address = split; address != runtime.end; address += 4u)
            Check(runtime.CompareItems(runtime.words.at(address), pivot) >= 0,
                  "right partition is no less than pivot");
        auto values = Values(runtime);
        std::sort(values.begin(), values.end());
        Check(values == std::vector<std::uint32_t>({1u, 2u, 3u, 4u, 5u, 6u}),
              "pivot partition preserves all pointers");
    }
}

}  // namespace

int main() {
    TestCompareThree();
    TestInsertion();
    TestPivotPartition();
    for (const auto count : {16u, 17u, 31u, 40u, 100u, 257u}) {
        TestPartition(count, false);
        TestPartition(count, true);
    }
}
