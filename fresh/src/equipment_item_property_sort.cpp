#include "pal4/equipment_item_property_sort.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace pal4 {
namespace {

std::int32_t Property(std::uint32_t item, std::uint32_t index,
                      IEquipmentItemPropertyRuntime& runtime) {
    return std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + 0x168u + 4u * index));
}

std::uint32_t MedianItem(const std::vector<std::uint32_t>& items,
                         std::size_t begin, std::size_t end,
                         std::uint32_t index,
                         IEquipmentItemPropertyRuntime& runtime) {
    const auto first = items[begin];
    const auto middle = items[begin + (end - begin) / 2u];
    const auto last = items[end - 1u];
    if (EquipmentItemPropertySort::Compare(first, middle, index, runtime)) {
        if (!EquipmentItemPropertySort::Compare(middle, last, index, runtime))
            return EquipmentItemPropertySort::Compare(first, last, index,
                                                       runtime) ? last : first;
        return middle;
    }
    if (EquipmentItemPropertySort::Compare(first, last, index, runtime))
        return first;
    return EquipmentItemPropertySort::Compare(middle, last, index, runtime) ?
        last : middle;
}

void InsertionSort(std::vector<std::uint32_t>& items,
                   std::size_t first, std::size_t last,
                   std::uint32_t index,
                   IEquipmentItemPropertyRuntime& runtime) {
    for (auto cursor = first; cursor < last; ++cursor) {
        const auto value = items[cursor];
        auto position = cursor;
        while (position > 0u && EquipmentItemPropertySort::Compare(
                   value, items[position - 1u], index, runtime)) {
            items[position] = items[position - 1u];
            --position;
        }
        items[position] = value;
    }
}

}  // namespace

bool EquipmentItemPropertySort::Compare(
    std::uint32_t left, std::uint32_t right,
    std::uint32_t property_index,
    IEquipmentItemPropertyRuntime& runtime) {
    const auto left_value = Property(left, property_index, runtime);
    const auto right_value = Property(right, property_index, runtime);
    return left_value < right_value;
}

std::size_t EquipmentItemPropertySort::Partition(
    std::vector<std::uint32_t>& items, std::size_t begin,
    std::size_t end, std::uint32_t pivot_item,
    std::uint32_t property_index,
    IEquipmentItemPropertyRuntime& runtime) {
    auto left = begin;
    auto right = end;
    for (;;) {
        while (Compare(items[left], pivot_item, property_index, runtime))
            ++left;
        do {
            --right;
        } while (Compare(pivot_item, items[right], property_index,
                         runtime));
        if (right <= left) return left;
        std::swap(items[left], items[right]);
        ++left;
    }
}

std::size_t EquipmentItemPropertySort::QuickSort(
    std::vector<std::uint32_t>& items, std::size_t begin,
    std::size_t end, std::uint32_t property_index,
    IEquipmentItemPropertyRuntime& runtime) {
    while (end - begin > 16u) {
        const auto pivot = MedianItem(items, begin, end,
                                      property_index, runtime);
        const auto cut = Partition(items, begin, end, pivot,
                                    property_index, runtime);
        if (end - cut > cut - begin) {
            (void)QuickSort(items, begin, cut, property_index, runtime);
            begin = cut;
        } else {
            (void)QuickSort(items, cut, end, property_index, runtime);
            end = cut;
        }
    }
    return end - begin;
}

std::vector<std::uint32_t>::iterator EquipmentItemPropertySort::Sort(
    std::vector<std::uint32_t>& items, std::uint32_t property_index,
    IEquipmentItemPropertyRuntime& runtime) {
    if (items.size() <= 16u) {
        InsertionSort(items, 1u, items.size(), property_index, runtime);
        return items.end();
    }

    auto begin = std::size_t{0u};
    auto end = items.size();
    do {
        const auto pivot = MedianItem(items, begin, end,
                                      property_index, runtime);
        const auto cut = Partition(items, begin, end, pivot,
                                    property_index, runtime);
        if (end - cut > cut - begin) {
            (void)QuickSort(items, begin, cut, property_index, runtime);
            begin = cut;
        } else {
            (void)QuickSort(items, cut, end, property_index, runtime);
            end = cut;
        }
    } while (end - begin > 16u);

    InsertionSort(items, 1u, 16u, property_index, runtime);
    InsertionSort(items, 16u, items.size(), property_index, runtime);
    return items.end();
}

}  // namespace pal4
