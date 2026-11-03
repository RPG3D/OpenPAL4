#include "pal4/attribute_value_conversion.h"

#include <bit>

namespace pal4 {

void AttributeValueRuntimeStub::CallMethod(
    std::uint32_t, AttributeValueState&, std::uint32_t) {}
std::int32_t AttributeValueRuntimeStub::CallSetter(
    std::uint32_t, AttributeValueState&, std::uint32_t) { return 0; }
const char* AttributeValueRuntimeStub::ResolveCString(std::uint32_t) {
    return "";
}
std::int64_t AttributeValueRuntimeStub::FloatToLong(float) { return 0; }
double AttributeValueRuntimeStub::GetDouble(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0.0; }
std::uint32_t AttributeValueRuntimeStub::GetWord(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t AttributeValueRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::int32_t AttributeValueRuntimeStub::FormatDouble(
    std::uint32_t, std::uint32_t, const char*, double) { return 0; }
std::int32_t AttributeValueRuntimeStub::FormatInt(
    std::uint32_t, std::uint32_t, const char*, std::int32_t) { return 0; }
float AttributeValueRuntimeStub::ParseFloat(std::uint32_t) { return 0.0f; }
std::int32_t AttributeValueRuntimeStub::ParseInt(std::uint32_t) { return 0; }
std::int32_t AttributeValueRuntimeStub::SetWord(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) { return 0; }

std::uint32_t AttributeValueConversion::CallMethodAndReturnProperty(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    runtime.CallMethod(target, attribute, 8u);
    return attribute.value;
}

std::int32_t AttributeValueConversion::DoubleToString(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = runtime.GetDouble(target, attribute.property_key, 16u);
    const auto result = runtime.FormatDouble(kFormatBuffer, 0x1ffu,
                                             "%f", value);
    attribute.value = kFormatBuffer;
    return result;
}

std::int32_t AttributeValueConversion::IntegerToString(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = static_cast<std::int32_t>(
        runtime.GetWord(target, attribute.property_key, 16u));
    const auto result = runtime.FormatInt(kFormatBuffer, 0x1ffu,
                                          "%d", value);
    attribute.value = kFormatBuffer;
    return result;
}

std::int32_t AttributeValueConversion::ByteToString(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = static_cast<std::int32_t>(
        runtime.GetWord(target, attribute.property_key, 16u) & 0xffu);
    const auto result = runtime.FormatInt(kFormatBuffer, 0x1ffu,
                                          "%d", value);
    attribute.value = kFormatBuffer;
    return result;
}

std::uint32_t AttributeValueConversion::GetString(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto record = runtime.GetWord(target, attribute.property_key, 16u);
    const auto data = runtime.ReadDword(record + 4u);
    attribute.value = data == 0u ? kEmptyLogMessage : data;
    return attribute.value;
}

std::uint32_t AttributeValueConversion::GetInteger(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto record = runtime.GetWord(target, attribute.property_key, 16u);
    attribute.value = runtime.ReadDword(record);
    return attribute.value;
}

std::int32_t AttributeValueConversion::SetFloatFromString(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = runtime.ParseFloat(attribute.value);
    return runtime.SetWord(target, attribute.property_key,
                           std::bit_cast<std::uint32_t>(value), 20u);
}

std::int32_t AttributeValueConversion::SetIntFromString(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = runtime.ParseInt(attribute.value);
    return runtime.SetWord(target, attribute.property_key,
                           static_cast<std::uint32_t>(value), 20u);
}

std::int32_t AttributeValueConversion::SetBoolFromString(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = runtime.ParseInt(attribute.value) > 0 ? 1u : 0u;
    return runtime.SetWord(target, attribute.property_key, value, 20u);
}

std::int32_t AttributeValueConversion::SetEnumFromString(
    const AttributeValueState& attribute,
    const PropertyCollectionState& collection, const std::uint32_t target,
    IAttributeValueRuntime& runtime,
    IPropertyAccessRuntime& property_runtime) {
    return PropertyAccess::ProcessByName(
        collection, target, runtime.ResolveCString(attribute.value),
        property_runtime);
}

std::int32_t AttributeValueConversion::SetFromAttribute(
    AttributeValueState& attribute, const std::uint32_t target,
    const std::uint32_t value, IAttributeValueRuntime& runtime) {
    attribute.value = value;
    return runtime.CallSetter(target, attribute, 12u);
}

double AttributeValueConversion::GetAsAttribute(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    runtime.CallMethod(target, attribute, 8u);
    return std::bit_cast<float>(attribute.value);
}

void AttributeValueConversion::GetFloatFromProperty(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = static_cast<float>(
        runtime.GetDouble(target, attribute.property_key, 16u));
    attribute.value = std::bit_cast<std::uint32_t>(value);
}

std::uint8_t AttributeValueConversion::GetBoolFromProperty(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = static_cast<std::uint8_t>(
        runtime.GetWord(target, attribute.property_key, 16u));
    attribute.value = std::bit_cast<std::uint32_t>(
        static_cast<float>(value));
    return value;
}

void AttributeValueConversion::GetFloatFromStringProperty(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto record = runtime.GetWord(target, attribute.property_key, 16u);
    const auto data = runtime.ReadDword(record + 4u);
    const auto text = data == 0u ? kEmptyLogMessage : data;
    attribute.value = std::bit_cast<std::uint32_t>(runtime.ParseFloat(text));
}

void AttributeValueConversion::GetFloatFromDirectStringProperty(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto record = runtime.GetWord(target, attribute.property_key, 16u);
    const auto text = runtime.ReadDword(record);
    attribute.value = std::bit_cast<std::uint32_t>(runtime.ParseFloat(text));
}

std::int32_t AttributeValueConversion::GetIntFromProperty(
    AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = static_cast<std::int32_t>(
        runtime.GetWord(target, attribute.property_key, 16u));
    attribute.value = std::bit_cast<std::uint32_t>(
        static_cast<float>(value));
    return value;
}

std::int32_t AttributeValueConversion::SetProperty(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    return runtime.SetWord(target, attribute.property_key,
                           attribute.value, 20u);
}

std::int32_t AttributeValueConversion::SetFloat(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = std::bit_cast<float>(attribute.value);
    const auto converted = runtime.FloatToLong(value);
    return runtime.SetWord(target, attribute.property_key,
                           static_cast<std::uint32_t>(converted), 20u);
}

std::int32_t AttributeValueConversion::SetBool(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeValueRuntime& runtime) {
    const auto value = std::bit_cast<float>(attribute.value);
    return runtime.SetWord(target, attribute.property_key,
                           value > 0.0f ? 1u : 0u, 20u);
}

}  // namespace pal4
