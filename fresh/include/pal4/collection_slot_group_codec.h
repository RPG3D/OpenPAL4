#pragma once

#include <cstdint>

#include "pal4/inventory_condition_record.h"

namespace pal4 {

class ICollectionSlotGroupCodecRuntime {
public:
    virtual ~ICollectionSlotGroupCodecRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ReleaseForWrite(std::uint32_t object,
                                 std::uint32_t stream) = 0;
    virtual void WriteSerializationFlag(std::uint32_t slot,
                                        std::uint32_t stream,
                                        const char* flag) = 0;
    virtual void ReleaseSerializationFlag(std::uint32_t slot) = 0;
    [[nodiscard]] virtual IInventoryConditionStreamRuntime& RecordRuntime() = 0;
    virtual void WriteStreamWord(std::uint32_t stream,
                                 std::uint32_t value) = 0;
    virtual void WriteStreamSpan(std::uint32_t stream,
                                 std::uint32_t address,
                                 std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CreateEmptyTemporaryTree() = 0;
    virtual void DestroyTemporaryTree(std::uint32_t tree) = 0;
    virtual void ReleaseForRead(std::uint32_t object,
                                std::uint32_t stream) = 0;
    [[nodiscard]] virtual bool ReadSerializationFlag(
        std::uint32_t slot, std::uint32_t stream,
        const char* expected) = 0;
    virtual void MarkSerializationError(std::uint32_t slot) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void ConstructRecord(std::uint32_t record) = 0;
    virtual void AppendRecord(std::uint32_t slot,
                              std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamWord(
        std::uint32_t stream) = 0;
    virtual void ReadStreamSpan(std::uint32_t stream,
                                std::uint32_t destination,
                                std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class CollectionSlotGroupCodec final {
public:
    // IDA 0x4c49d0. Returns the low byte from the seventh slot writer.
    [[nodiscard]] static std::uint8_t Serialize(
        std::uint32_t object, std::uint32_t stream,
        ICollectionSlotGroupCodecRuntime& runtime);
    // IDA 0x4c4da0. Writes the flag, count and each pointed-to record.
    [[nodiscard]] static std::uint8_t SerializeSlot(
        std::uint32_t slot, std::uint32_t stream,
        ICollectionSlotGroupCodecRuntime& runtime);
    // IDA 0x4c4c60. Returns the low byte from the seventh slot reader.
    [[nodiscard]] static std::uint8_t Deserialize(
        std::uint32_t object, std::uint32_t stream,
        ICollectionSlotGroupCodecRuntime& runtime);
    // IDA 0x4c4e90. Returns zero on flag mismatch, else last record result.
    [[nodiscard]] static std::uint8_t DeserializeSlot(
        std::uint32_t slot, std::uint32_t stream,
        ICollectionSlotGroupCodecRuntime& runtime);
};

}  // namespace pal4
