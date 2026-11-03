#include "pal4/script_property_float_adapter.h"

#include <array>

namespace pal4 {
namespace {

ScriptForwardResult ReadAdjusted(std::uint32_t object, std::uint32_t first,
                                 std::uint32_t second, float adjustment,
                                 IScriptPropertyFloatRuntime& runtime) {
    const auto link = runtime.ReadWord(object + 8u);
    const std::array<std::uint32_t, 2> arguments{first, second};
    auto result = runtime.CallVirtual(link, 0xc8u, arguments);
    if (second == 0u) result.st0 += static_cast<double>(adjustment);
    return result;
}

}  // namespace

std::uint32_t ScriptPropertyFloatAdapter::Construct(
    std::uint32_t object, IScriptPropertyFloatRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteByte(object + 12u, 0u);
    runtime.WriteWord(object, 0x847f70u);
    return object;
}

void ScriptPropertyFloatAdapter::ClearFlag(
    std::uint32_t object, IScriptPropertyFloatRuntime& runtime) {
    runtime.WriteByte(object + 12u, 0u);
}

void ScriptPropertyFloatAdapter::SetFlag(
    std::uint32_t object, IScriptPropertyFloatRuntime& runtime) {
    runtime.WriteByte(object + 12u, 1u);
}

ScriptForwardResult ScriptPropertyFloatAdapter::ForwardAtC8(
    std::uint32_t object, ScriptPropertyForwarders::Arguments arguments,
    IScriptPropertyFloatRuntime& runtime) {
    const auto link = runtime.ReadWord(object + 8u);
    return runtime.CallVirtual(link, 0xc8u, arguments);
}

ScriptForwardResult ScriptPropertyFloatAdapter::ReadPlus005(
    std::uint32_t object, std::uint32_t first, std::uint32_t second,
    IScriptPropertyFloatRuntime& runtime) {
    return ReadAdjusted(object, first, second, 0.05f, runtime);
}

ScriptForwardResult ScriptPropertyFloatAdapter::ReadPlus010(
    std::uint32_t object, std::uint32_t first, std::uint32_t second,
    IScriptPropertyFloatRuntime& runtime) {
    return ReadAdjusted(object, first, second, 0.1f, runtime);
}

ScriptForwardResult ScriptPropertyFloatAdapter::ReadPlus020(
    std::uint32_t object, std::uint32_t first, std::uint32_t second,
    IScriptPropertyFloatRuntime& runtime) {
    return ReadAdjusted(object, first, second, 0.2f, runtime);
}

}  // namespace pal4
