#pragma once

#include "pal4/property_access.h"

#include <cstdint>

namespace pal4 {

struct AttributeValueState {
    std::uint32_t vtable{};
    std::uint32_t value{};
    std::uint32_t property_key{};
};
static_assert(sizeof(AttributeValueState) == 12u);

class IAttributeValueRuntime {
public:
    virtual ~IAttributeValueRuntime() = default;
    virtual void CallMethod(std::uint32_t target,
                            AttributeValueState& attribute,
                            std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::int32_t CallSetter(
        std::uint32_t target, AttributeValueState& attribute,
        std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual const char* ResolveCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int64_t FloatToLong(float value) = 0;
    [[nodiscard]] virtual double GetDouble(std::uint32_t target,
                                            std::uint32_t key,
                                            std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t GetWord(
        std::uint32_t target, std::uint32_t key,
        std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FormatDouble(
        std::uint32_t buffer, std::uint32_t count, const char* format,
        double value) = 0;
    [[nodiscard]] virtual std::int32_t FormatInt(
        std::uint32_t buffer, std::uint32_t count, const char* format,
        std::int32_t value) = 0;
    [[nodiscard]] virtual float ParseFloat(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::int32_t ParseInt(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::int32_t SetWord(
        std::uint32_t target, std::uint32_t key, std::uint32_t value,
        std::uint32_t vtable_offset) = 0;
};

class AttributeValueRuntimeStub final : public IAttributeValueRuntime {
public:
    void CallMethod(std::uint32_t target, AttributeValueState& attribute,
                    std::uint32_t vtable_offset) override;
    std::int32_t CallSetter(std::uint32_t target,
                            AttributeValueState& attribute,
                            std::uint32_t vtable_offset) override;
    const char* ResolveCString(std::uint32_t address) override;
    std::int64_t FloatToLong(float value) override;
    double GetDouble(std::uint32_t target, std::uint32_t key,
                     std::uint32_t vtable_offset) override;
    std::uint32_t GetWord(std::uint32_t target, std::uint32_t key,
                          std::uint32_t vtable_offset) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::int32_t FormatDouble(std::uint32_t buffer, std::uint32_t count,
                              const char* format, double value) override;
    std::int32_t FormatInt(std::uint32_t buffer, std::uint32_t count,
                           const char* format, std::int32_t value) override;
    float ParseFloat(std::uint32_t text) override;
    std::int32_t ParseInt(std::uint32_t text) override;
    std::int32_t SetWord(std::uint32_t target, std::uint32_t key,
                         std::uint32_t value,
                         std::uint32_t vtable_offset) override;
};

class AttributeValueConversion final {
public:
    static constexpr std::uint32_t kFormatBuffer = 0x92f6f0u;
    static constexpr std::uint32_t kEmptyLogMessage = 0x841964u;

    // IDA 0x5eda40..0x5edc00.
    [[nodiscard]] static std::uint32_t CallMethodAndReturnProperty(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t DoubleToString(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t IntegerToString(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t ByteToString(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetString(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetInteger(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t SetFloatFromString(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t SetIntFromString(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t SetBoolFromString(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    // IDA 0x5edd80..0x5edf00.
    [[nodiscard]] static std::int32_t SetEnumFromString(
        const AttributeValueState& attribute,
        const PropertyCollectionState& collection,
        std::uint32_t target, IAttributeValueRuntime& runtime,
        IPropertyAccessRuntime& property_runtime);
    [[nodiscard]] static std::int32_t SetFromAttribute(
        AttributeValueState& attribute, std::uint32_t target,
        std::uint32_t value, IAttributeValueRuntime& runtime);
    [[nodiscard]] static double GetAsAttribute(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    static void GetFloatFromProperty(AttributeValueState& attribute,
                                     std::uint32_t target,
                                     IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetBoolFromProperty(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    static void GetFloatFromStringProperty(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    static void GetFloatFromDirectStringProperty(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t GetIntFromProperty(
        AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t SetProperty(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t SetFloat(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
    [[nodiscard]] static std::int32_t SetBool(
        const AttributeValueState& attribute, std::uint32_t target,
        IAttributeValueRuntime& runtime);
};

}  // namespace pal4
