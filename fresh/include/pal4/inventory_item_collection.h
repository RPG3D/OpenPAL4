#pragma once

#include <cstdint>

#include "pal4/inventory_db_record_codec.h"
#include "pal4/inventory_record_memory.h"

namespace pal4 {

class IInventoryItemRuntime : public IInventoryRecordMemoryRuntime {
public:
    virtual ~IInventoryItemRuntime() = default;
    virtual void LogInvalidSlot(std::int32_t index,
                                std::uint32_t source_line) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameData(std::uint32_t id) = 0;
    virtual void ObserveItem(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void AppendRecent(std::uint32_t recent,
                              std::uint32_t sequence) = 0;
};

class IInventoryItemStreamRuntime : public IInventoryDbRecordRuntime {
public:
    [[nodiscard]] virtual std::uint32_t TemporaryRecordAddress() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class InventoryItemCollection final {
public:
    // IDA 0x4c7000 / 0x4c71d0: add and remove item quantities.
    [[nodiscard]] static std::uint32_t Add(
        std::uint32_t object, std::uint32_t id, std::int32_t count,
        IInventoryItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t object, std::uint32_t id, std::int32_t count,
        IInventoryItemRuntime& runtime);
    // IDA 0x4c72b0 / 0x4c7310: search a selected or inferred slot.
    [[nodiscard]] static std::uint32_t FindBySequence(
        std::uint32_t object, std::int32_t sequence,
        std::uint32_t index, IInventoryItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindById(
        std::uint32_t object, std::uint32_t id,
        std::uint32_t index, IInventoryItemRuntime& runtime);
    // IDA 0x4c7360 / 0x4c73a0: count and reverse-indexed access.
    [[nodiscard]] static std::int32_t Count(
        std::uint32_t object, std::int32_t index,
        IInventoryItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t AtReverse(
        std::uint32_t object, std::int32_t index,
        std::uint32_t reverse_offset, IInventoryItemRuntime& runtime);
    // IDA 0x4c7400 / 0x4c7410: recent-key field access.
    [[nodiscard]] static std::uint32_t SetRecentKey(
        std::uint32_t object, std::int32_t index,
        std::uint32_t sequence, IInventoryItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindRecent(
        std::uint32_t object, std::uint32_t index,
        IInventoryItemRuntime& runtime);
    // IDA 0x4c7430: five slots with separate invalid-index log lines.
    [[nodiscard]] static std::uint32_t SlotAddress(
        std::uint32_t object, std::int32_t index,
        IInventoryItemRuntime& runtime);
    // IDA 0x4c7480 / 0x4c74f0: sorted sequence and linear item-ID search.
    [[nodiscard]] static std::uint32_t FindSequenceInSlot(
        std::uint32_t slot, std::int32_t sequence,
        IInventoryItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindIdInSlot(
        std::uint32_t slot, std::uint32_t id,
        IInventoryItemRuntime& runtime);
    // IDA 0x4c7520: erase one sorted record by sequence.
    [[nodiscard]] static bool EraseSequence(
        std::uint32_t slot, std::int32_t sequence,
        IInventoryItemRuntime& runtime);
    // IDA 0x4c75d0 / 0x4c77f0: record count and 12-byte record streams.
    [[nodiscard]] static std::uint8_t DeserializeSlot(
        std::uint32_t slot, std::uint32_t stream,
        IInventoryItemStreamRuntime& runtime);
    [[nodiscard]] static std::uint8_t SerializeSlot(
        std::uint32_t slot, std::uint32_t stream,
        IInventoryItemStreamRuntime& runtime);
    // IDA 0x4c7c10: insert a 12-byte record at a selected position.
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t slot, std::uint32_t position,
        std::uint32_t game_data, std::uint32_t sequence,
        std::uint32_t count, IInventoryItemRuntime& runtime);
    // IDA 0x4c7df0: throw the vector subscript exception.
    [[noreturn]] static void ThrowInvalidElement();
};

}  // namespace pal4
