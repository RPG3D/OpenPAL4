#include "pal4/skill_event_sorter.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using Entry = pal4::SkillEventRegistration;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Less(Entry left, Entry right) {
    const auto left_id = static_cast<std::int32_t>(left.id);
    const auto right_id = static_cast<std::int32_t>(right.id);
    return left_id < right_id ||
           (left_id == right_id && left.callback < right.callback);
}

bool Same(const std::vector<Entry>& left, const std::vector<Entry>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0u; i < left.size(); ++i)
        if (left[i].id != right[i].id ||
            left[i].callback != right[i].callback)
            return false;
    return true;
}

void ComparisonAndMedian() {
    using pal4::SkillEventSorter;
    Check(SkillEventSorter::CompareValues({1u, 1u}, {1u, 2u}) &&
          !SkillEventSorter::CompareValues({1u, 2u}, {1u, 1u}) &&
          SkillEventSorter::CompareValues({0xffffffffu, 9u}, {0u, 0u}),
          "comparison uses signed ID and unsigned callback tie-break");
    const Entry values[]{{1u, 10u}, {2u, 20u}, {3u, 30u}};
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b)
            for (int c = 0; c < 3; ++c) {
                const auto median = SkillEventSorter::SelectMedian(
                    values[a], values[b], values[c]);
                auto expected = std::vector<Entry>{values[a], values[b], values[c]};
                std::sort(expected.begin(), expected.end(), Less);
                Check(median.id == expected[1].id &&
                      median.callback == expected[1].callback,
                      "median selection handles all value orders and duplicates");
            }
}

void PartitionAndInsertion() {
    using pal4::SkillEventSorter;
    std::vector<Entry> entries{{3u, 4u}, {1u, 9u}, {2u, 1u},
                               {2u, 0u}, {4u, 2u}, {0xffffffffu, 8u}};
    auto expected = entries;
    std::sort(expected.begin(), expected.end(), Less);
    const Entry pivot{2u, 1u};
    const auto split = SkillEventSorter::Partition(entries, pivot);
    Check(split > 0u && split < entries.size(),
          "partition produces two nonempty ranges for an internal pivot");
    for (std::size_t i = 0u; i < split; ++i)
        Check(!Less(pivot, entries[i]), "left partition is <= pivot");
    for (std::size_t i = split; i < entries.size(); ++i)
        Check(!Less(entries[i], pivot), "right partition is >= pivot");
    SkillEventSorter::InsertionSort(entries);
    Check(Same(entries, expected), "insertion sort completes the partition");
    std::vector<Entry> empty;
    bool threw = false;
    try { static_cast<void>(SkillEventSorter::Partition(empty, pivot)); }
    catch (const std::invalid_argument&) { threw = true; }
    Check(threw, "empty partition has a defined failure path");
}

void FullSort() {
    using pal4::SkillEventSorter;
    for (const auto size : {0u, 1u, 16u, 17u, 39u, 128u}) {
        std::vector<Entry> entries;
        for (std::uint32_t i = 0u; i < size; ++i)
            entries.push_back({static_cast<std::uint32_t>(
                                   static_cast<std::int32_t>((i * 13u) % 19u) - 9),
                               (i * 71u) % 23u});
        auto expected = entries;
        std::sort(expected.begin(), expected.end(), Less);
        SkillEventSorter::Sort(entries);
        Check(Same(entries, expected),
              "native threshold sorting matches signed pair ordering");
    }
    std::vector<Entry> entries;
    for (std::uint32_t i = 0u; i < 80u; ++i)
        entries.push_back({i % 7u, 80u - i});
    auto expected = entries;
    std::sort(expected.begin(), expected.end(), Less);
    Check(SkillEventSorter::SortLargePartitions(entries) <= 16u,
          "large partition recursion stops at native 16-item threshold");
    SkillEventSorter::InsertionSort(entries);
    Check(Same(entries, expected),
          "large partition phase retains all records for insertion cleanup");
}

}  // namespace

int main() {
    try {
        ComparisonAndMedian();
        PartitionAndInsertion();
        FullSort();
        std::cout << "skill event sorter tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
