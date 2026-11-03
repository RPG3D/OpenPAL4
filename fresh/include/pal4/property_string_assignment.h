#pragma once

#include <cstdint>

namespace pal4 {

class IPropertyStringRuntime {
public:
    virtual ~IPropertyStringRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t AllocateText(
        std::uint32_t record, std::uint32_t length, std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t record, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t DetachAndReset(
        std::uint32_t record) = 0;
    [[noreturn]] virtual void ThrowRange() = 0;
};

class PropertyStringAssignment final {
public:
    static constexpr std::uint32_t kEmptyLogMessage = 0x841964u;

    // IDA 0x5ed8f0. The property object stores the record offset at +4.
    [[nodiscard]] static std::uint8_t Assign(
        std::uint32_t record_offset, std::uint32_t target,
        std::uint32_t source_record, IPropertyStringRuntime& runtime);
    // IDA 0x5eda30.
    [[nodiscard]] static std::uint32_t ElementAddress(
        std::uint32_t record_offset, std::uint32_t target) noexcept;
};

}  // namespace pal4
