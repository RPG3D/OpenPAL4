#pragma once

#include "pal4/inventory_archive.h"
#include "pal4/native_game_save_archive_buffer.h"

#include <cstdint>

namespace pal4 {

class NativeInventorySession;

// Writes the live inventory image into a caller-owned in-memory save archive.
class NativeInventorySaveRuntime final : public IInventoryArchiveRuntime,
                                         public IInventorySlotStreamRuntime {
public:
    static constexpr std::uint32_t kArchiveHandle = 1u;

    NativeInventorySaveRuntime(NativeInventorySession& inventory,
                               NativeGameSaveArchiveBuffer& archive);
    [[nodiscard]] std::uint8_t Serialize();

    [[nodiscard]] std::uint32_t CreateFlag(const char* text) override;
    void ReleaseFlag(std::uint32_t flag) override;
    void WriteFlag(std::uint32_t stream, std::uint32_t flag) override;
    [[nodiscard]] bool ReadFlag(std::uint32_t stream, std::uint32_t flag,
                                const char* expected) override;
    void MarkFlagError(std::uint32_t flag) override;
    [[nodiscard]] std::uint32_t CreateDocument(const char* type) override;
    void DestroyDocument(std::uint32_t document) override;
    [[nodiscard]] std::uint32_t FindAttribute(
        std::uint32_t document, const char* name) override;
    void SetAttributeFromField(std::uint32_t attribute,
                               std::uint32_t field) override;
    void AddAttributeFromField(std::uint32_t document,
                               const char* name, std::uint32_t field) override;
    [[nodiscard]] std::uint32_t ReadAttributeInt(
        std::uint32_t attribute) override;
    void WriteDocument(std::uint32_t document,
                       std::uint32_t stream) override;
    [[nodiscard]] bool ReadDocument(std::uint32_t document,
                                    std::uint32_t stream) override;
    [[nodiscard]] IInventorySlotStreamRuntime& ArrayRuntime() override;
    void SerializeSlots(std::uint32_t slots,
                        std::uint32_t stream) override;
    [[nodiscard]] std::uint8_t SerializeBase(
        std::uint32_t object, std::uint32_t stream) override;
    void DeserializeSlots(std::uint32_t slots,
                          std::uint32_t stream) override;
    [[nodiscard]] std::uint8_t DeserializeBase(
        std::uint32_t object, std::uint32_t stream) override;

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    void FreeMemory(std::uint32_t address) override;
    void WriteStreamWord(std::uint32_t stream,
                         std::uint32_t value) override;
    [[nodiscard]] std::uint32_t ReadStreamWord(
        std::uint32_t stream) override;
    void ResetSlots(std::uint32_t slots) override;
    void ResetArray(std::uint32_t array) override;
    [[nodiscard]] std::uint32_t ResetBase(
        std::uint32_t object) override;

private:
    void RequireArchive(std::uint32_t stream) const;
    void RequireDocument(std::uint32_t document) const;

    NativeInventorySession& inventory_;
    NativeGameSaveArchiveBuffer& archive_;
    BinaryXmlNode document_;
    bool document_open_{};
    const char* flag_text_{};
};

}  // namespace pal4
