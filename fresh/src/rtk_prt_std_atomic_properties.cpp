#include "pal4/rtk_prt_std_atomic_properties.h"
#include "pal4/effect_color_math.h"

#include <cstddef>

namespace pal4 {
namespace {

std::uint8_t Active(const RtkPrtStdAtomicObjectState& state) {
    return static_cast<std::uint8_t>(state.words[1u]);
}

std::uint8_t GetTriple(
    const RtkPrtStdAtomicObjectState& state,
    std::array<std::uint32_t, 3>& output, const std::uint32_t offset,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    for (std::uint32_t i = 0u; i != 3u; ++i)
        output[i] = runtime.ReadWord(resource + offset + i * 4u);
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint8_t SetTriple(
    const RtkPrtStdAtomicObjectState& state,
    std::array<float, 3> values, const bool relative,
    const std::uint32_t offset,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    if (relative) {
        for (std::uint32_t i = 0u; i != 3u; ++i)
            values[i] = runtime.ReadFloat(resource + offset + i * 4u) +
                        values[i];
    }
    for (std::uint32_t i = 0u; i != 3u; ++i)
        runtime.WriteFloat(resource + offset + i * 4u, values[i]);
    return runtime.UpdateFrame(resource);
}

}  // namespace

std::uint8_t RtkPrtStdAtomicProperties::GetRotation(
    const RtkPrtStdAtomicObjectState& state,
    std::array<std::uint32_t, 3>& output,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    return GetTriple(state, output, 112u, runtime);
}

std::uint8_t RtkPrtStdAtomicProperties::GetScale(
    const RtkPrtStdAtomicObjectState& state,
    std::array<std::uint32_t, 3>& output,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    return GetTriple(state, output, 100u, runtime);
}

std::uint8_t RtkPrtStdAtomicProperties::GetPosition(
    const RtkPrtStdAtomicObjectState& state,
    std::array<std::uint32_t, 3>& output,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    return GetTriple(state, output, 112u, runtime);
}

std::uint8_t RtkPrtStdAtomicProperties::SetPosition(
    const RtkPrtStdAtomicObjectState& state,
    const std::array<float, 3> values, const bool relative,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    return SetTriple(state, values, relative, 112u, runtime);
}

std::uint8_t RtkPrtStdAtomicProperties::SetRotation(
    const RtkPrtStdAtomicObjectState& state,
    const std::array<float, 3> values, const bool relative,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    return SetTriple(state, values, relative, 88u, runtime);
}

std::uint8_t RtkPrtStdAtomicProperties::SetScale(
    const RtkPrtStdAtomicObjectState& state,
    const std::array<float, 3> values, const bool relative,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    return SetTriple(state, values, relative, 100u, runtime);
}

std::uint8_t RtkPrtStdAtomicProperties::SetResourceMode(
    const RtkPrtStdAtomicObjectState& state, const std::uint8_t value,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    runtime.WriteByte(resource + 3u, value);
    return static_cast<std::uint8_t>(resource);
}

std::uint8_t RtkPrtStdAtomicProperties::GetColor(
    const RtkPrtStdAtomicObjectState& state,
    const std::array<std::uint8_t*, 4>& output,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    for (std::uint32_t i = 0u; i != 4u; ++i) {
        const auto value = runtime.ReadFloat(resource + 212u + i * 4u);
        *output[i] = static_cast<std::uint8_t>(
            static_cast<std::int32_t>(value * 255.0f + 0.5f));
    }
    return static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(output[3u]));
}

std::uint8_t RtkPrtStdAtomicProperties::SetColor(
    const RtkPrtStdAtomicObjectState& state,
    const std::array<std::uint8_t, 4>& color,
    IRtkPrtStdAtomicPropertiesRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    if (runtime.ReadByte(resource + 316u) != 0u)
        runtime.WriteByte(resource + 316u, 0u);
    for (std::uint32_t i = 0u; i != 4u; ++i) {
        runtime.WriteFloat(resource + 212u + i * 4u,
            NormalizeEffectColorByte(color[i]));
    }
    return static_cast<std::uint8_t>(resource);
}

}  // namespace pal4
