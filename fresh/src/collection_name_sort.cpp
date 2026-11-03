#include "pal4/collection_name_sort.h"

namespace pal4 {

std::uint32_t CollectionNameSort::CompareThree(
    const std::uint32_t first, const std::uint32_t middle,
    const std::uint32_t last,
    ICollectionPointerSortRuntime& runtime) {
    if (runtime.CompareItems(first, middle) < 0) {
        if (runtime.CompareItems(middle, last) < 0) return middle;
        return runtime.CompareItems(first, last) < 0 ? last : first;
    }
    if (runtime.CompareItems(first, last) < 0) return first;
    return runtime.CompareItems(middle, last) < 0 ? last : middle;
}

std::uint32_t CollectionNameSort::Partition(
    std::uint32_t begin, std::uint32_t end, const std::uint32_t pivot,
    ICollectionPointerSortRuntime& runtime) {
    for (;;) {
        while (runtime.CompareItems(runtime.ReadWord(begin), pivot) < 0)
            begin += 4u;
        do {
            end -= 4u;
        } while (runtime.CompareItems(pivot, runtime.ReadWord(end)) < 0);
        if (end <= begin) return begin;
        const auto value = runtime.ReadWord(begin);
        runtime.WriteWord(begin, runtime.ReadWord(end));
        runtime.WriteWord(end, value);
        begin += 4u;
    }
}

std::uint32_t CollectionNameSort::PartitionLarge(
    std::uint32_t begin, std::uint32_t end,
    ICollectionPointerSortRuntime& runtime) {
    while ((end - begin) / 4u > 16u) {
        const auto middle = begin + ((end - begin) / 8u) * 4u;
        const auto pivot = CompareThree(runtime.ReadWord(begin),
                                        runtime.ReadWord(middle),
                                        runtime.ReadWord(end - 4u), runtime);
        const auto left = Partition(begin, end, pivot, runtime);

        if (left == begin || left == end) return (end - begin) / 4u;
        if (end - left > left - begin) {
            (void)PartitionLarge(begin, left, runtime);
            begin = left;
        } else {
            (void)PartitionLarge(left, end, runtime);
            end = left;
        }
    }
    return (end - begin) / 4u;
}

void CollectionNameSort::InsertionSort(
    const std::uint32_t begin, const std::uint32_t end,
    ICollectionPointerSortRuntime& runtime) {
    if (begin == end) return;
    for (auto current = begin + 4u; current != end; current += 4u) {
        const auto candidate = runtime.ReadWord(current);
        auto destination = current;
        while (destination > begin &&
               runtime.CompareItems(candidate,
                                    runtime.ReadWord(destination - 4u)) < 0) {
            runtime.WriteWord(destination,
                              runtime.ReadWord(destination - 4u));
            destination -= 4u;
        }
        runtime.WriteWord(destination, candidate);
    }
}

}  // namespace pal4
