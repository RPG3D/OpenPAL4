#include "pal4/equipment_item_property_sort.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentItemPropertyRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void Set(std::uint32_t item, std::uint32_t index,
             std::int32_t value) {
        words[item + 0x168u + 4u * index] =
            std::bit_cast<std::uint32_t>(value);
    }
    std::int32_t Get(std::uint32_t item, std::uint32_t index) const {
        return std::bit_cast<std::int32_t>(
            words.at(item + 0x168u + 4u * index));
    }
};

void TestComparator() {
    Runtime runtime;
    runtime.Set(0x1000u, 2u, -3);
    runtime.Set(0x2000u, 2u, 5);
    runtime.Set(0x3000u, 2u, -3);
    Check(pal4::EquipmentItemPropertySort::Compare(
              0x1000u, 0x2000u, 2u, runtime),
          "negative property compares below positive property");
    Check(!pal4::EquipmentItemPropertySort::Compare(
              0x2000u, 0x1000u, 2u, runtime) &&
              !pal4::EquipmentItemPropertySort::Compare(
              0x1000u, 0x3000u, 2u, runtime),
          "comparator is strict and signed");
}

void TestPartitionAndQuickSort() {
    Runtime runtime;
    std::vector<std::uint32_t> items;
    for (std::uint32_t index = 0u; index < 48u; ++index) {
        const auto item = 0x1000u + index * 0x100u;
        runtime.Set(item, 1u,
                    static_cast<std::int32_t>((index * 19u) % 31u) - 15);
        items.push_back(item);
    }
    auto before = items;
    const auto pivot_item = items[24u];
    const auto pivot = runtime.Get(pivot_item, 1u);
    const auto cut = pal4::EquipmentItemPropertySort::Partition(
        items, 0u, items.size(), pivot_item, 1u, runtime);
    Check(cut > 0u && cut < items.size(),
          "partition returns interior cut");
    for (std::size_t index = 0u; index < cut; ++index)
        Check(runtime.Get(items[index], 1u) <= pivot,
              "partition left side is at most pivot");
    for (std::size_t index = cut; index < items.size(); ++index)
        Check(runtime.Get(items[index], 1u) >= pivot,
              "partition right side is at least pivot");
    std::sort(before.begin(), before.end());
    auto after = items;
    std::sort(after.begin(), after.end());
    Check(after == before, "partition preserves all item addresses");

    const auto remaining = pal4::EquipmentItemPropertySort::QuickSort(
        items, 0u, items.size(), 1u, runtime);
    Check(remaining <= 16u, "quicksort stops at insertion-sort threshold");
    after = items;
    std::sort(after.begin(), after.end());
    Check(after == before, "quicksort preserves all items");
}

void TestSortAllSizes() {
    for (std::uint32_t size = 0u; size <= 128u; ++size) {
        Runtime runtime;
        std::vector<std::uint32_t> items;
        for (std::uint32_t index = 0u; index < size; ++index) {
            const auto item = 0x1000u + index * 0x100u;
            const auto value = static_cast<std::int32_t>(
                ((index * 37u + size * 11u) % 43u)) - 21;
            runtime.Set(item, 3u, value);
            items.push_back(item);
        }
        std::reverse(items.begin(), items.end());
        auto before = items;
        const auto result = pal4::EquipmentItemPropertySort::Sort(
            items, 3u, runtime);
        Check(result == items.end(), "sort returns end iterator");
        for (std::size_t index = 1u; index < items.size(); ++index)
            Check(runtime.Get(items[index - 1u], 3u) <=
                      runtime.Get(items[index], 3u),
                  "sort orders signed properties for 0-128 items");
        std::sort(before.begin(), before.end());
        auto after = items;
        std::sort(after.begin(), after.end());
        Check(before == after, "sort preserves exact item multiset");
    }
}

void TestSmallStableInsertion() {
    Runtime runtime;
    std::vector<std::uint32_t> items{0x1000u, 0x2000u, 0x3000u,
                                      0x4000u, 0x5000u};
    runtime.Set(0x1000u, 0u, 2);
    runtime.Set(0x2000u, 0u, 1);
    runtime.Set(0x3000u, 0u, 1);
    runtime.Set(0x4000u, 0u, 2);
    runtime.Set(0x5000u, 0u, -1);
    (void)pal4::EquipmentItemPropertySort::Sort(items, 0u, runtime);
    Check(items == std::vector<std::uint32_t>{
              0x5000u, 0x2000u, 0x3000u, 0x1000u, 0x4000u},
          "small insertion path preserves order of equal properties");
}

}  // namespace

int main() {
    TestComparator();
    TestPartitionAndQuickSort();
    TestSortAllSizes();
    TestSmallStableInsertion();
}
