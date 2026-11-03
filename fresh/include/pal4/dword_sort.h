#pragma once

#include <cstdint>

namespace pal4 {

class DwordSort final {
public:
    using Compare = bool (*)(std::uint32_t left, std::uint32_t right);

    // IDA 0x4422a0: partitions ranges larger than 16 DWORDs.
    [[nodiscard]] static std::int32_t PartitionLarge(
        std::uint32_t* begin, std::uint32_t* end, Compare compare);

    // IDA 0x4423c0: completes ordering using the first element as a sentinel.
    [[nodiscard]] static std::uint32_t* InsertionSort(
        std::uint32_t* begin, std::uint32_t* end, Compare compare);

    // IDA 0x4774a0: partition large ranges, then finish with insertion sort.
    // The original EAX is the count for 0/1 elements and end for larger ranges.
    [[nodiscard]] static std::uintptr_t Sort(
        std::uint32_t* begin, std::uint32_t* end, Compare compare);
};

}  // namespace pal4
