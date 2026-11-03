#include "pal4/effect_system_object_f_access.h"

namespace pal4 {
namespace {

bool IsActive(const EffectSystemObjectFState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

}  // namespace

std::uint8_t EffectSystemObjectFAccess::GetScale(
    const EffectSystemObjectFState& state,
    std::array<std::uint32_t, 3>& output,
    IEffectSystemObjectFAccessRuntime& runtime) {
    if (!IsActive(state))
        return 0u;
    const auto object = state.words[29u];
    output[0u] = runtime.ReadWord(object + 40u);
    output[1u] = runtime.ReadWord(object + 44u);
    output[2u] = 0u;
    return static_cast<std::uint8_t>(output[1u]);
}

std::uint8_t EffectSystemObjectFAccess::GetRotation(
    const EffectSystemObjectFState& state,
    std::array<std::uint32_t, 3>& output,
    const std::uint8_t third_output_address_low_byte,
    IEffectSystemObjectFAccessRuntime& runtime) {
    if (!IsActive(state))
        return 0u;
    output[0u] = runtime.ReadWord(state.words[29u] + 24u);
    output[1u] = 0u;
    output[2u] = 0u;
    return third_output_address_low_byte;
}

std::uint8_t EffectSystemObjectFAccess::GetPosition(
    const EffectSystemObjectFState& state,
    std::array<std::uint32_t, 3>& output,
    IEffectSystemObjectFAccessRuntime& runtime) {
    if (!IsActive(state))
        return 0u;
    const auto object = state.words[29u];
    for (std::uint32_t index = 0u; index < output.size(); ++index)
        output[index] = runtime.ReadWord(object + 28u + 4u * index);
    return static_cast<std::uint8_t>(output[2u]);
}

}  // namespace pal4
