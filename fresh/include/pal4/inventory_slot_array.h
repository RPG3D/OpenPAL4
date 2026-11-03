#pragma once

#include <cstdint>

namespace pal4 {

class IInventorySlotArrayRuntime {
public:
    virtual ~IInventorySlotArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ConstructElement(std::uint32_t element) = 0;
    virtual void DestroyElement(std::uint32_t element) = 0;
};

class IInventorySlotQueryRuntime {
public:
    virtual ~IInventorySlotQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameDbInstance() = 0;
    [[nodiscard]] virtual std::uint32_t FindGameData(
        std::uint32_t collection, std::uint32_t id) = 0;
    virtual void LogInvalidIndex(std::int32_t index,
                                 std::uint32_t source_line) = 0;
};

class IInventorySlotStreamRuntime {
public:
    virtual ~IInventorySlotStreamRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void WriteStreamWord(std::uint32_t stream,
                                 std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamWord(
        std::uint32_t stream) = 0;
};

class InventorySlotArray final {
public:
    // IDA 0x4c65d0: three 16-byte element constructors.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t array, IInventorySlotArrayRuntime& runtime);
    // IDA 0x4c65f0: reset ends, then destroy in reverse order.
    static void Cleanup(std::uint32_t array,
                        IInventorySlotArrayRuntime& runtime);
    // IDA 0x4c6650: returns the third element's old end pointer.
    [[nodiscard]] static std::uint32_t ResetEnds(
        std::uint32_t array, IInventorySlotArrayRuntime& runtime);
    // IDA 0x4c6680: look up the ID and insert under its GameDB slot index.
    [[nodiscard]] static std::uint32_t ProcessGameData(
        std::uint32_t array, std::uint32_t id,
        IInventorySlotQueryRuntime& runtime);
    // IDA 0x4c66b0: insert into one indexed sorted slot.
    [[nodiscard]] static std::uint32_t InsertByIndex(
        std::uint32_t array, std::int32_t index,
        std::uint32_t id, IInventorySlotQueryRuntime& runtime);
    // IDA 0x4c66e0: exact signed-ID search in the selected slot.
    [[nodiscard]] static bool Contains(
        std::uint32_t array, std::int32_t id,
        IInventorySlotQueryRuntime& runtime);
    // IDA 0x4c6760 / 0x4c6790: indexed size and value access.
    [[nodiscard]] static std::int32_t Size(
        std::uint32_t array, std::int32_t index,
        IInventorySlotQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ValueAt(
        std::uint32_t array, std::int32_t index,
        std::uint32_t offset, IInventorySlotQueryRuntime& runtime);
    // IDA 0x4c6c90: throw the vector subscript exception used by ValueAt.
    [[noreturn]] static void ThrowInvalidSubscript();
    // IDA 0x4c67e0: three 16-byte slots with distinct error lines.
    [[nodiscard]] static std::uint32_t SlotAddress(
        std::uint32_t array, std::int32_t index,
        IInventorySlotQueryRuntime& runtime);
    // IDA 0x4c6840: insert a unique signed ID, growing slot storage.
    [[nodiscard]] static std::uint32_t InsertSorted(
        std::uint32_t slot, std::uint32_t id,
        IInventorySlotQueryRuntime& runtime);
    // IDA 0x4c69e0 / 0x4c6a40: write three slots or one slot.
    [[nodiscard]] static std::uint8_t SerializeAll(
        std::uint32_t array, std::uint32_t stream,
        IInventorySlotStreamRuntime& runtime);
    [[nodiscard]] static std::uint8_t SerializeSlot(
        std::uint32_t slot, std::uint32_t stream,
        IInventorySlotStreamRuntime& runtime);
    // IDA 0x4c6a10 / 0x4c6ab0: read three slots or one slot.
    [[nodiscard]] static std::uint8_t DeserializeAll(
        std::uint32_t array, std::uint32_t stream,
        IInventorySlotStreamRuntime& runtime);
    [[nodiscard]] static std::uint8_t DeserializeSlot(
        std::uint32_t slot, std::uint32_t stream,
        IInventorySlotStreamRuntime& runtime);
};

}  // namespace pal4
