#include "pal4/effect_camera_transform_updater.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

constexpr std::array<CameraVector3, 3> kAxes{
    CameraVector3{1.0f, 0.0f, 0.0f},
    CameraVector3{0.0f, 1.0f, 0.0f},
    CameraVector3{0.0f, 0.0f, 1.0f}};

}  // namespace

std::uint32_t EffectCameraTransformUpdater::Update(
    const EffectCameraObjectState& state,
    IEffectCameraTransformRuntime& runtime) {
    const auto frame = state.words[31u];
    if (frame == 0u)
        return 0u;
    if (ReadByte(state, 44u) != 0u)
        return frame;

    const std::array<std::uint32_t, 3> position{
        state.words[25u], state.words[26u], state.words[27u]};
    const std::array<float, 3> rotation{
        std::bit_cast<float>(state.words[22u]),
        std::bit_cast<float>(state.words[23u]),
        std::bit_cast<float>(state.words[24u])};
    if (ReadByte(state, 47u) != 0u) {
        auto direction = EffectCameraPosition::ResolveLookAt(state, runtime);
        for (std::uint32_t index = 0u; index < 3u; ++index)
            direction[index] -= std::bit_cast<float>(position[index]);
        const auto matrix =
            EffectCameraRotationBasis::BuildMatrixFromDirection(direction,
                                                                runtime);
        runtime.SetFrameIdentity(frame);
        runtime.ApplyMatrix(frame, matrix, 0u);
        static_cast<void>(runtime.UpdateFrameObjects(frame));
        const auto transform = runtime.ObjectTransform(frame);
        CameraVector3 axis{};
        for (std::uint32_t index = 0u; index < 3u; ++index)
            axis[index] = runtime.ReadFloat(transform + 32u + 4u * index);
        runtime.RotateFrame(frame, axis, rotation[2u], 2u);
    } else {
        runtime.SetFrameIdentity(frame);
        for (std::uint32_t index = 0u; index < 3u; ++index)
            runtime.RotateFrame(frame, kAxes[index], rotation[index],
                                index == 0u ? 0u : 2u);
    }
    runtime.TranslateFrame(frame, position, 2u);
    return runtime.UpdateFrameObjects(frame);
}

}  // namespace pal4
