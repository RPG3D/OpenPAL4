#include "pal4/dword_partition_sort.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kWordBytes = 4;
constexpr std::uint32_t kInsertionThreshold = 16;

std::uint32_t Median(std::uint32_t first, std::uint32_t middle,
                     std::uint32_t last,
                     IDwordPartitionSortRuntime& runtime) {
    if (runtime.Less(first, middle)) {
        if (runtime.Less(middle, last)) return middle;
        return runtime.Less(first, last) ? last : first;
    }
    if (runtime.Less(first, last)) return first;
    return runtime.Less(middle, last) ? last : middle;
}

}  // namespace

std::uint32_t DwordPartitionSort::SortLarge(
    std::uint32_t begin, std::uint32_t end,
    IDwordPartitionSortRuntime& runtime) {
    while ((end - begin) / kWordBytes > kInsertionThreshold) {
        const auto count = (end - begin) / kWordBytes;
        const auto pivot = Median(
            runtime.ReadWord(begin),
            runtime.ReadWord(begin + (count / 2) * kWordBytes),
            runtime.ReadWord(end - kWordBytes), runtime);
        const auto middle = Partition(begin, end, pivot, runtime);
        if (end - middle > middle - begin) {
            (void)SortLarge(begin, middle, runtime);
            begin = middle;
        } else {
            (void)SortLarge(middle, end, runtime);
            end = middle;
        }
    }
    return (end - begin) / kWordBytes;
}

std::uint32_t DwordPartitionSort::Partition(
    std::uint32_t begin, std::uint32_t end, std::uint32_t pivot,
    IDwordPartitionSortRuntime& runtime) {
    while (true) {
        while (runtime.Less(runtime.ReadWord(begin), pivot))
            begin += kWordBytes;
        do {
            end -= kWordBytes;
        } while (runtime.Less(pivot, runtime.ReadWord(end)));
        if (end <= begin) return begin;
        const auto left = runtime.ReadWord(begin);
        runtime.WriteWord(begin, runtime.ReadWord(end));
        runtime.WriteWord(end, left);
        begin += kWordBytes;
    }
}

}  // namespace pal4
