#pragma once

#include <cstdint>

namespace pal4 {

class IInventoryRecordMemoryRuntime {
public:
    virtual ~IInventoryRecordMemoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class InventoryRecordMemory final {
public:
    // IDA 0x4c7ed0: construct copies for a half-open record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t destination, IInventoryRecordMemoryRuntime& runtime);
    // IDA 0x4c7f10: construct repeated copies of one record.
    [[nodiscard]] static std::uint32_t FillConstruct(
        std::uint32_t destination, std::int32_t count,
        std::uint32_t source, IInventoryRecordMemoryRuntime& runtime);
    // IDA 0x4c7f40: assign one record to a half-open range.
    [[nodiscard]] static std::uint32_t FillAssign(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t source, IInventoryRecordMemoryRuntime& runtime);
    // IDA 0x4c7f80: copy a range backwards into destination_end.
    [[nodiscard]] static std::uint32_t CopyBackward(
        std::uint32_t first, std::uint32_t last,
        std::uint32_t destination_end,
        IInventoryRecordMemoryRuntime& runtime);
    // IDA 0x4c7fc0: copy-construct one record when destination is nonnull.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        IInventoryRecordMemoryRuntime& runtime);
};

}  // namespace pal4
