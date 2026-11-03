#pragma once

#include <cstdint>

#include "pal4/inventory_subsystem.h"
#include "pal4/inventory_slot_array.h"

namespace pal4 {

class IInventoryArchiveRuntime : public IInventoryResetRuntime {
public:
    [[nodiscard]] virtual std::uint32_t CreateFlag(
        const char* initial_text) = 0;
    virtual void ReleaseFlag(std::uint32_t flag) = 0;
    virtual void WriteFlag(std::uint32_t stream,
                           std::uint32_t flag) = 0;
    [[nodiscard]] virtual bool ReadFlag(std::uint32_t stream,
                                        std::uint32_t flag,
                                        const char* expected) = 0;
    virtual void MarkFlagError(std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t CreateDocument(
        const char* document_type) = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t document, const char* name) = 0;
    virtual void SetAttributeFromField(std::uint32_t attribute,
                                       std::uint32_t field) = 0;
    virtual void AddAttributeFromField(std::uint32_t document,
                                       const char* name,
                                       std::uint32_t field) = 0;
    [[nodiscard]] virtual std::uint32_t ReadAttributeInt(
        std::uint32_t attribute) = 0;
    virtual void WriteDocument(std::uint32_t document,
                               std::uint32_t stream) = 0;
    [[nodiscard]] virtual bool ReadDocument(std::uint32_t document,
                                             std::uint32_t stream) = 0;
    [[nodiscard]] virtual IInventorySlotStreamRuntime& ArrayRuntime() = 0;
    virtual void SerializeSlots(std::uint32_t slots,
                                std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint8_t SerializeBase(
        std::uint32_t object, std::uint32_t stream) = 0;
    virtual void DeserializeSlots(std::uint32_t slots,
                                  std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint8_t DeserializeBase(
        std::uint32_t object, std::uint32_t stream) = 0;
};

class InventoryArchive final {
public:
    static constexpr const char* kFlag = "IY_SERIALIZE_FLAG";
    static constexpr const char* kDocument = "INVENTORY";
    static constexpr const char* kMoney = "money";

    // IDA 0x4c6130: write flag, document, money and three member regions.
    [[nodiscard]] static std::uint8_t Serialize(
        std::uint32_t object, std::uint32_t stream,
        IInventoryArchiveRuntime& runtime);
    // IDA 0x4c6330: validate flag/document, reset, then read members.
    [[nodiscard]] static std::uint8_t Deserialize(
        std::uint32_t object, std::uint32_t stream,
        IInventoryArchiveRuntime& runtime);
};

}  // namespace pal4
