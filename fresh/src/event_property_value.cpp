#include "pal4/event_property_value.h"

namespace pal4 {

std::uint32_t EventPropertyValueRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyValueRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
void EventPropertyValueRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
void EventPropertyValueRuntimeStub::IncrementTempOff() {}
void EventPropertyValueRuntimeStub::DecrementTempOff() {}
std::uint32_t EventPropertyValueRuntimeStub::FloatBits(std::uint32_t) {
    return 0u;
}
std::int32_t EventPropertyValueRuntimeStub::IntegerValue(std::uint32_t) {
    return 0;
}
std::uint32_t EventPropertyValueRuntimeStub::StringRecord(std::uint32_t) {
    return 0u;
}
std::uint32_t EventPropertyValueRuntimeStub::SetValue(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
void EventPropertyValueRuntimeStub::InitializeString(StringBufferState&,
                                                      std::uint32_t) {}
void EventPropertyValueRuntimeStub::SetEnumValue(
    std::uint32_t, std::uint32_t, const StringBufferState&) {}
std::uint32_t EventPropertyValueRuntimeStub::CleanupString(
    StringBufferState&) { return 0u; }

std::uint32_t EventPropertyValue::Create(
    const std::uint32_t owner, const std::uint32_t operand,
    const std::uint32_t vtable, IEventPropertyValueRuntime& runtime) {
    const auto property = runtime.Allocate(8u);
    if (property == 0u) {
        runtime.WriteWord(owner + 4u, 0u);
        return 0u;
    }
    runtime.WriteWord(property, 0x849898u);
    runtime.IncrementTempOff();
    runtime.WriteWord(property, vtable);
    runtime.WriteWord(property + 4u, operand);
    runtime.WriteWord(owner + 4u, property);
    return operand;
}

std::uint32_t EventPropertyValue::CreateFloat(
    const std::uint32_t owner, const std::uint32_t operand,
    IEventPropertyValueRuntime& runtime) {
    return Create(owner, operand, 0x8498d8u, runtime);
}
std::uint32_t EventPropertyValue::CreateInt(
    const std::uint32_t owner, const std::uint32_t operand,
    IEventPropertyValueRuntime& runtime) {
    return Create(owner, operand, 0x8498e8u, runtime);
}
std::uint32_t EventPropertyValue::CreateString(
    const std::uint32_t owner, const std::uint32_t operand,
    IEventPropertyValueRuntime& runtime) {
    return Create(owner, operand, 0x8498f8u, runtime);
}
std::uint32_t EventPropertyValue::CreateEnum(
    const std::uint32_t owner, const std::uint32_t operand,
    IEventPropertyValueRuntime& runtime) {
    return Create(owner, operand, 0x849908u, runtime);
}
std::uint32_t EventPropertyValue::CreateBool(
    const std::uint32_t owner, const std::uint32_t operand,
    IEventPropertyValueRuntime& runtime) {
    return Create(owner, operand, 0x849918u, runtime);
}
std::uint32_t EventPropertyValue::CreateUnknown(
    const std::uint32_t owner, const std::uint32_t operand,
    IEventPropertyValueRuntime& runtime) {
    return Create(owner, operand, 0x849928u, runtime);
}

std::uint32_t EventPropertyValue::SetFloatFromFloat(
    const std::uint32_t owner, const std::uint32_t key,
    const std::uint32_t source, IEventPropertyValueRuntime& runtime) {
    const auto property = runtime.ReadWord(owner + 4u);
    return runtime.SetValue(property, key, runtime.FloatBits(source));
}

std::uint32_t EventPropertyValue::SetIntFromInt(
    const std::uint32_t owner, const std::uint32_t key,
    const std::uint32_t source, IEventPropertyValueRuntime& runtime) {
    const auto property = runtime.ReadWord(owner + 4u);
    return runtime.SetValue(property, key,
                            static_cast<std::uint32_t>(
                                runtime.IntegerValue(source)));
}

std::uint32_t EventPropertyValue::SetStringFromString(
    const std::uint32_t owner, const std::uint32_t key,
    const std::uint32_t source, IEventPropertyValueRuntime& runtime) {
    const auto property = runtime.ReadWord(owner + 4u);
    return runtime.SetValue(property, key, runtime.StringRecord(source));
}

std::uint32_t EventPropertyValue::SetEnumFromString(
    const std::uint32_t owner, const std::uint32_t key,
    const std::uint32_t source, IEventPropertyValueRuntime& runtime) {
    const auto record = runtime.StringRecord(source);
    auto text = runtime.ReadWord(record + 4u);
    if (text == 0u) text = StringBuffer::EmptyLogMessageAddress();
    StringBufferState temporary{};
    runtime.InitializeString(temporary, text);
    try {
        runtime.SetEnumValue(runtime.ReadWord(owner + 4u), key, temporary);
    } catch (...) {
        (void)runtime.CleanupString(temporary);
        throw;
    }
    return runtime.CleanupString(temporary);
}

std::uint32_t EventPropertyValue::SetBoolFromInt(
    const std::uint32_t owner, const std::uint32_t key,
    const std::uint32_t source, IEventPropertyValueRuntime& runtime) {
    const auto property = runtime.ReadWord(owner + 4u);
    const auto value = runtime.IntegerValue(source);
    return runtime.SetValue(property, key,
                            (source & 0xffffff00u) |
                                (value > 0 ? 1u : 0u));
}

std::uint32_t EventPropertyValue::GetType() { return 3u; }

void EventPropertyValue::Cleanup(
    const std::uint32_t property, IEventPropertyValueRuntime& runtime) {
    runtime.WriteWord(property, 0x849898u);
    runtime.DecrementTempOff();
}

}  // namespace pal4
