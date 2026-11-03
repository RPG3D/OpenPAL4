#include "pal4/effect_camera_position.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

}  // namespace

CameraVector3 EffectCameraPosition::ResolveLookAt(
    const EffectCameraObjectState& state,
    IEffectCameraPositionRuntime& runtime) {
    CameraVector3 position{
        std::bit_cast<float>(state.words[12u]),
        std::bit_cast<float>(state.words[13u]),
        std::bit_cast<float>(state.words[14u])};
    if (ReadByte(state, 60u) != 0u) {
        const auto frame = runtime.ReadWord(state.words[31u] + 4u);
        if (frame != 0u) {
            auto target = runtime.GetTargetPosition(state, 64u);
            const auto transform = runtime.ObjectTransform(frame);
            for (std::uint32_t index = 0u; index < 3u; ++index)
                target[index] -= runtime.ReadFloat(
                    transform + 48u + 4u * index);
            CameraMatrixWords matrix{};
            runtime.SetupMatrix(matrix, transform);
            runtime.MultiplyMatrix(matrix, frame + 16u);
            runtime.TransformPoint(target, matrix);

            auto direction = runtime.GetTargetLookAt(state, 64u);
            for (std::uint32_t index = 0u; index < 3u; ++index)
                direction[index] -= position[index];
            runtime.TransformVector(direction, matrix);
            for (std::uint32_t index = 0u; index < 3u; ++index)
                position[index] = target[index] - direction[index];
        }
    }
    return position;
}

std::uint8_t EffectCameraPosition::GetLookAtWorld(
    const EffectCameraObjectState& state,
    std::array<std::uint32_t, 3>& output,
    const std::uint8_t third_output_address_low_byte,
    IEffectCameraPositionRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 80u) == 0u)
        return 0u;
    const auto position = ResolveLookAt(state, runtime);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        output[index] = std::bit_cast<std::uint32_t>(position[index]);
    return third_output_address_low_byte;
}

}  // namespace pal4
