#pragma once

#include "pal4/collection_pointer_group.h"

#include <cstdint>

namespace pal4 {

class CollectionNameSort final {
public:
    // IDA 0x5ef000.
    [[nodiscard]] static std::uint32_t CompareThree(
        std::uint32_t first, std::uint32_t middle, std::uint32_t last,
        ICollectionPointerSortRuntime& runtime);
    // IDA 0x5ef1c0.
    [[nodiscard]] static std::uint32_t Partition(
        std::uint32_t begin, std::uint32_t end, std::uint32_t pivot,
        ICollectionPointerSortRuntime& runtime);
    // IDA 0x5eed00. Returns the size of the final unpartitioned range.
    [[nodiscard]] static std::uint32_t PartitionLarge(
        std::uint32_t begin, std::uint32_t end,
        ICollectionPointerSortRuntime& runtime);
    // IDA 0x5eeec0.
    static void InsertionSort(std::uint32_t begin, std::uint32_t end,
                              ICollectionPointerSortRuntime& runtime);
};

}  // namespace pal4
