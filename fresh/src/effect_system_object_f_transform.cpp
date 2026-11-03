#include "pal4/effect_system_object_f_transform.h"

#include <bit>

namespace pal4 {
namespace {

bool Active(const EffectSystemObjectFState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

float ReadFloat(std::uint32_t address,
                IEffectSystemObjectFTransformRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

float Approach(float current, float target, float step) {
    if (current > target) {
        const auto next = current - step;
        return next < target ? target : next;
    }
    if (current < target) {
        const auto next = current + step;
        return next > target ? target : next;
    }
    return current;
}

}  // namespace

std::uint32_t EffectSystemObjectFTransform::SetScale(
    const EffectSystemObjectFState& state, const float width,
    const float height, const bool relative,
    IEffectSystemObjectFTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto object = state.words[29u];
    const auto new_width = relative ?
        ReadFloat(object + 40u, runtime) + width : width;
    const auto new_height = relative ?
        ReadFloat(object + 44u, runtime) + height : height;
    runtime.WriteWord(object + 40u, Bits(new_width));
    const auto result = Bits(new_height);
    runtime.WriteWord(object + 44u, result);
    return result;
}

std::uint32_t EffectSystemObjectFTransform::SetRotation(
    const EffectSystemObjectFState& state, const float angle,
    const bool relative, IEffectSystemObjectFTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto object = state.words[29u];
    const auto value = relative ?
        ReadFloat(object + 24u, runtime) + angle : angle;
    runtime.WriteWord(object + 24u, Bits(value));
    return relative ? object : 0u;
}

std::uint32_t EffectSystemObjectFTransform::SetPosition(
    const EffectSystemObjectFState& state,
    const std::array<float, 3>& position, const bool relative,
    IEffectSystemObjectFTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto object = state.words[29u];
    std::array<float, 3> values = position;
    if (relative) {
        for (std::uint32_t index = 0u; index < values.size(); ++index)
            values[index] += ReadFloat(object + 28u + 4u * index, runtime);
    }
    for (std::uint32_t index = 0u; index < values.size(); ++index)
        runtime.WriteWord(object + 28u + 4u * index, Bits(values[index]));
    return runtime.InitializeTransform(object);
}

std::uint32_t EffectSystemObjectFTransform::ApplyMotion(
    EffectSystemObjectFState& state,
    const std::array<float, 3>& target,
    const std::array<float, 3>& angles,
    const std::array<float, 3>& steps,
    IEffectSystemObjectFTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;

    const auto frame = runtime.GetFrame(state);
    const auto matrix = frame + 16u;
    std::array<float, 3> translation{};
    for (std::uint32_t index = 0u; index < translation.size(); ++index)
        translation[index] = target[index] * -1.0f;
    runtime.TranslateMatrix(matrix, translation, 2u);
    constexpr std::array<std::uint32_t, 3> axes{
        0x84a08cu, 0x84a098u, 0x84a0a4u};
    for (std::uint32_t index = 0u; index < axes.size(); ++index)
        runtime.RotateMatrix(matrix, axes[index], angles[index], 2u);
    for (auto& value : translation)
        value *= -1.0f;
    runtime.TranslateMatrix(matrix, translation, 2u);
    runtime.UpdateFrameObjects(frame);

    auto position = runtime.ReadFramePosition(frame);
    for (std::uint32_t index = 0u; index < position.size(); ++index)
        position[index] = Approach(position[index], target[index],
                                   steps[index]);
    return SetPosition(state, position, false, runtime);
}

std::uint8_t EffectSystemObjectFTransform::SetPositionWords(
    const EffectSystemObjectFState& state,
    const std::array<std::uint32_t, 3>& position,
    IEffectSystemObjectFTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto object = state.words[29u];
    for (std::uint32_t index = 0u; index < position.size(); ++index)
        runtime.WriteWord(object + 28u + 4u * index, position[index]);
    return static_cast<std::uint8_t>(runtime.InitializeTransform(object));
}

}  // namespace pal4
