#include "pal4/event_property_binding.h"

#include "pal4/string_buffer.h"

#include <bit>

namespace pal4 {

std::uint32_t EventPropertyBindingRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyBindingRuntimeStub::ReadValue(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
double EventPropertyBindingRuntimeStub::ReadDouble(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0.0; }
std::uint32_t EventPropertyBindingRuntimeStub::NameRecord(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint8_t EventPropertyBindingRuntimeStub::WriteFloat(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint8_t EventPropertyBindingRuntimeStub::WriteInteger(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint8_t EventPropertyBindingRuntimeStub::WriteString(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t EventPropertyBindingRuntimeStub::ConfigString(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t EventPropertyBindingRuntimeStub::ParseFloatBits(
    std::uint32_t) { return 0u; }
std::uint32_t EventPropertyBindingRuntimeStub::ParseInteger(
    std::uint32_t) { return 0u; }
std::uint32_t EventPropertyBindingRuntimeStub::SetSourceValue(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {
    return 0u;
}

namespace {

std::uint32_t Name(const std::uint32_t source,
                   IEventPropertyBindingRuntime& runtime) {
    const auto record = runtime.NameRecord(source, 4u);
    const auto data = runtime.ReadWord(record + 4u);
    return data == 0u ? StringBuffer::EmptyLogMessageAddress() : data;
}

}  // namespace

std::uint8_t EventPropertyBinding::WriteFloat(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto key = runtime.ReadWord(wrapper + 8u);
    const auto value = static_cast<float>(
        runtime.ReadDouble(source, key, 16u));
    const auto name = Name(source, runtime);
    return runtime.WriteFloat(runtime.ReadWord(wrapper + 4u), name,
                              std::bit_cast<std::uint32_t>(value));
}

std::uint8_t EventPropertyBinding::WriteString(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto key = runtime.ReadWord(wrapper + 8u);
    const auto record = runtime.ReadValue(source, key, 16u);
    auto value = runtime.ReadWord(record + 4u);
    if (value == 0u) value = StringBuffer::EmptyLogMessageAddress();
    const auto name = Name(source, runtime);
    return runtime.WriteString(runtime.ReadWord(wrapper + 4u), name, value);
}

std::uint8_t EventPropertyBinding::WritePointerString(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto key = runtime.ReadWord(wrapper + 8u);
    const auto value = runtime.ReadWord(runtime.ReadValue(source, key, 16u));
    const auto name = Name(source, runtime);
    return runtime.WriteString(runtime.ReadWord(wrapper + 4u), name, value);
}

std::uint8_t EventPropertyBinding::WriteByte(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto key = runtime.ReadWord(wrapper + 8u);
    const auto value = runtime.ReadValue(source, key, 16u) & 0xffu;
    const auto name = Name(source, runtime);
    return runtime.WriteInteger(runtime.ReadWord(wrapper + 4u), name, value);
}

std::uint8_t EventPropertyBinding::WriteInteger(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto key = runtime.ReadWord(wrapper + 8u);
    const auto value = runtime.ReadValue(source, key, 16u);
    const auto name = Name(source, runtime);
    return runtime.WriteInteger(runtime.ReadWord(wrapper + 4u), name, value);
}

std::uint32_t EventPropertyBinding::SetFloatFromConfig(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto name = Name(source, runtime);
    const auto target = runtime.ReadWord(wrapper + 4u);
    const auto text = runtime.ConfigString(target, name);
    if (text == 0u) return 0u;
    const auto bits = runtime.ParseFloatBits(text);
    return runtime.SetSourceValue(source, runtime.ReadWord(wrapper + 8u),
                                   bits, 20u);
}

std::uint32_t EventPropertyBinding::SetBoolFromConfig(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto name = Name(source, runtime);
    const auto text = runtime.ConfigString(runtime.ReadWord(wrapper + 4u), name);
    if (text == 0u) return 0u;
    const auto parsed = runtime.ParseInteger(text);
    const auto value = (parsed & 0xffffff00u) |
                       (static_cast<std::int32_t>(parsed) > 0 ? 1u : 0u);
    return runtime.SetSourceValue(source, runtime.ReadWord(wrapper + 8u),
                                  value, 20u);
}

std::uint32_t EventPropertyBinding::SetIntegerFromConfig(
    const std::uint32_t wrapper, const std::uint32_t source,
    IEventPropertyBindingRuntime& runtime) {
    const auto name = Name(source, runtime);
    const auto text = runtime.ConfigString(runtime.ReadWord(wrapper + 4u), name);
    if (text == 0u) return 0u;
    const auto value = runtime.ParseInteger(text);
    return runtime.SetSourceValue(source, runtime.ReadWord(wrapper + 8u),
                                  value, 20u);
}

}  // namespace pal4
