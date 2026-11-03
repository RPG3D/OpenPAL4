#include "pal4/effect_camera_constraints.h"

#include <array>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

float Approach(const float value, const float target, const float step) {
    if (value == target)
        return value;
    if (value < target) {
        const float next = value + step;
        return next < target ? next : target;
    }
    const float next = value - step;
    return next < target ? target : next;
}

}  // namespace

std::uint32_t EffectCameraConstraints::Apply(
    EffectCameraObjectState& state, const CameraVector3& pivot,
    const CameraVector3& rotation, const CameraVector3& step,
    IEffectCameraConstraintsRuntime& runtime) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    const auto ready = ReadByte(state, 80u);
    if (ready == 0u)
        return ready;
    const auto inherit = ReadByte(state, 44u);
    if (inherit != 0u)
        return inherit;

    runtime.UpdateTransform(state);
    const std::uint32_t matrix = state.words[31u] + 16u;
    CameraVector3 offset{};
    for (std::uint32_t index = 0u; index < 3u; ++index)
        offset[index] = pivot[index] * -1.0f;
    runtime.Translate(matrix, offset, 2u);
    const std::array<CameraVector3, 3> axes{
        CameraVector3{1.0f, 0.0f, 0.0f},
        CameraVector3{0.0f, 1.0f, 0.0f},
        CameraVector3{0.0f, 0.0f, 1.0f}};
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.Rotate(matrix, axes[index], rotation[index], 2u);
    for (float& component : offset)
        component *= -1.0f;
    runtime.Translate(matrix, offset, 2u);

    auto position = runtime.MatrixTranslation(matrix);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        position[index] = Approach(position[index], pivot[index], step[index]);
    return runtime.SetPosition(state, position);
}

}  // namespace pal4
