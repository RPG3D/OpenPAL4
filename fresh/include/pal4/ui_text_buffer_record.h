#pragma once

#include <cstdint>

namespace pal4 {

class IUiTextBufferRecordRuntime {
public:
    virtual ~IUiTextBufferRecordRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    virtual void ThrowRange() = 0;
    virtual void DetachAndReset(std::uint32_t record) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t record, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
};

class UiTextBufferRecord final {
public:
    // IDA 0x4c16a0. Destination is a 20-byte record.
    static void InitializeAndCopy(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);

    // Shared 16-byte text portion used by IDA 0x4c16a0 and 0x5908a0.
    static void InitializeTextOnly(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
};

}  // namespace pal4
