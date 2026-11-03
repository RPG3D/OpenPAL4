#pragma once

#include <cstdint>

namespace pal4 {

class IDwordPartitionSortRuntime {
public:
    virtual ~IDwordPartitionSortRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool Less(std::uint32_t left,
                                    std::uint32_t right) = 0;
};

class DwordPartitionSort final {
public:
    // IDA 0x4dd3f0: quicksort partitions larger than sixteen DWORDs.
    [[nodiscard]] static std::uint32_t SortLarge(
        std::uint32_t begin, std::uint32_t end,
        IDwordPartitionSortRuntime& runtime);
    // IDA 0x4dd510: partition a DWORD range around a supplied pivot value.
    [[nodiscard]] static std::uint32_t Partition(
        std::uint32_t begin, std::uint32_t end, std::uint32_t pivot,
        IDwordPartitionSortRuntime& runtime);
};

}  // namespace pal4
