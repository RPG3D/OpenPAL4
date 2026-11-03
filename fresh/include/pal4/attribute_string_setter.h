#pragma once

#include "pal4/attribute_value_conversion.h"
#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

struct AttributeTemporaryStringState {
    std::uint32_t flag_word{};
    StringBufferState text{};
};
static_assert(sizeof(AttributeTemporaryStringState) == 16u);

class IAttributeStringSetterRuntime {
public:
    virtual ~IAttributeStringSetterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FormatFloat(
        std::uint32_t buffer, std::uint32_t count,
        const char* format, double value) = 0;
    [[nodiscard]] virtual std::uint8_t AllocateText(
        StringBufferState& text, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t length) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void DispatchTemporary(std::uint32_t target, std::uint32_t key,
                                   AttributeTemporaryStringState& temporary,
                                   std::uint32_t vtable_offset) = 0;
    virtual void InitializeString(StringBufferState& text,
                                  std::uint32_t source) = 0;
    virtual void DispatchString(std::uint32_t target, std::uint32_t key,
                                StringBufferState& text,
                                std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupString(
        StringBufferState& text) = 0;
};

class AttributeStringSetterRuntimeStub final
    : public IAttributeStringSetterRuntime {
public:
    std::uint32_t CStringLength(std::uint32_t address) override;
    std::int32_t FormatFloat(std::uint32_t buffer, std::uint32_t count,
                             const char* format, double value) override;
    std::uint8_t AllocateText(StringBufferState& text,
                              std::uint32_t length,
                              std::uint8_t mode) override;
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t Free(std::uint32_t address) override;
    void DispatchTemporary(std::uint32_t target, std::uint32_t key,
                           AttributeTemporaryStringState& temporary,
                           std::uint32_t vtable_offset) override;
    void InitializeString(StringBufferState& text,
                          std::uint32_t source) override;
    void DispatchString(std::uint32_t target, std::uint32_t key,
                        StringBufferState& text,
                        std::uint32_t vtable_offset) override;
    std::uint32_t CleanupString(StringBufferState& text) override;
};

class AttributeStringSetter final {
public:
    static constexpr std::uint32_t kFormatBuffer = 0x92f6f0u;
    // IDA 0x5edc30. The high flag bytes originate from uninitialized stack.
    [[nodiscard]] static std::uint32_t SetString(
        const AttributeValueState& attribute, std::uint32_t target,
        std::uint32_t initial_flag_word,
        IAttributeStringSetterRuntime& runtime);
    // IDA 0x5edd10.
    [[nodiscard]] static std::uint32_t SetStringFromString(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeStringSetterRuntime& runtime);
    // IDA 0x5edf30 / 0x5ee030.
    [[nodiscard]] static std::uint32_t SetFloatToString(
        const AttributeValueState& attribute, std::uint32_t target,
        std::uint32_t initial_flag_word,
        IAttributeStringSetterRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetFloatToStringBuffer(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeStringSetterRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t ReleaseTemporary(
        AttributeTemporaryStringState& temporary,
        IAttributeStringSetterRuntime& runtime);
};

}  // namespace pal4
