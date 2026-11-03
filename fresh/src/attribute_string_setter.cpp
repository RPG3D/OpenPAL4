#include "pal4/attribute_string_setter.h"

#include <bit>

namespace pal4 {

std::uint32_t AttributeStringSetterRuntimeStub::CStringLength(std::uint32_t) {
    return 0u;
}
std::int32_t AttributeStringSetterRuntimeStub::FormatFloat(
    std::uint32_t, std::uint32_t, const char*, double) { return 0; }
std::uint8_t AttributeStringSetterRuntimeStub::AllocateText(
    StringBufferState&, std::uint32_t, std::uint8_t) { return 0u; }
void AttributeStringSetterRuntimeStub::Copy(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void AttributeStringSetterRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint8_t AttributeStringSetterRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
std::uint32_t AttributeStringSetterRuntimeStub::Free(std::uint32_t address) {
    return address;
}
void AttributeStringSetterRuntimeStub::DispatchTemporary(
    std::uint32_t, std::uint32_t, AttributeTemporaryStringState&,
    std::uint32_t) {}
void AttributeStringSetterRuntimeStub::InitializeString(
    StringBufferState&, std::uint32_t) {}
void AttributeStringSetterRuntimeStub::DispatchString(
    std::uint32_t, std::uint32_t, StringBufferState&,
    std::uint32_t) {}
std::uint32_t AttributeStringSetterRuntimeStub::CleanupString(
    StringBufferState&) { return 0u; }

std::uint32_t AttributeStringSetter::ReleaseTemporary(
    AttributeTemporaryStringState& temporary,
    IAttributeStringSetterRuntime& runtime) {
    const auto data = temporary.text.data;
    if (data == 0u) return 0u;
    const auto header = data - 1u;
    const auto references = runtime.ReadByte(header);
    if (references == 0u || references == 0xffu)
        return runtime.Free(header);
    runtime.WriteByte(header,
        static_cast<std::uint8_t>(references - 1u));
    return header;
}

std::uint32_t AttributeStringSetter::SetString(
    const AttributeValueState& attribute, const std::uint32_t target,
    const std::uint32_t initial_flag_word,
    IAttributeStringSetterRuntime& runtime) {
    AttributeTemporaryStringState temporary;
    temporary.flag_word = (initial_flag_word & ~0xffu) | (target & 0xffu);
    const auto length = runtime.CStringLength(attribute.value);
    if (runtime.AllocateText(temporary.text, length, 1u) != 0u) {
        runtime.Copy(temporary.text.data, attribute.value, length);
        temporary.text.length = length;
        runtime.WriteByte(temporary.text.data + length, 0u);
    }
    try {
        runtime.DispatchTemporary(target, attribute.property_key,
                                  temporary, 20u);
    } catch (...) {
        (void)ReleaseTemporary(temporary, runtime);
        throw;
    }
    return ReleaseTemporary(temporary, runtime);
}

std::uint32_t AttributeStringSetter::SetStringFromString(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeStringSetterRuntime& runtime) {
    StringBufferState temporary;
    runtime.InitializeString(temporary, attribute.value);
    try {
        runtime.DispatchString(target, attribute.property_key,
                               temporary, 20u);
    } catch (...) {
        (void)runtime.CleanupString(temporary);
        throw;
    }
    return runtime.CleanupString(temporary);
}

std::uint32_t AttributeStringSetter::SetFloatToString(
    const AttributeValueState& attribute, const std::uint32_t target,
    const std::uint32_t initial_flag_word,
    IAttributeStringSetterRuntime& runtime) {
    const auto value = std::bit_cast<float>(attribute.value);
    (void)runtime.FormatFloat(kFormatBuffer, 0x1ffu, "%f", value);
    auto formatted = attribute;
    formatted.value = kFormatBuffer;
    return SetString(formatted, target, initial_flag_word, runtime);
}

std::uint32_t AttributeStringSetter::SetFloatToStringBuffer(
    const AttributeValueState& attribute, const std::uint32_t target,
    IAttributeStringSetterRuntime& runtime) {
    const auto value = std::bit_cast<float>(attribute.value);
    (void)runtime.FormatFloat(kFormatBuffer, 0x1ffu, "%f", value);
    auto formatted = attribute;
    formatted.value = kFormatBuffer;
    return SetStringFromString(formatted, target, runtime);
}

}  // namespace pal4
