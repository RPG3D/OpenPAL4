#include "pal4/rtk_prt_std_atomic_aim_frame.h"

#include <bit>

namespace pal4 {

std::uint8_t RtkPrtStdAtomicAimFrame::Update(
    RtkPrtStdAtomicObjectState& state, const std::uint8_t enabled,
    IRtkPrtStdAtomicAimRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    state.words[21u] = (state.words[21u] & 0xffffff00u) | enabled;
    if (enabled == 0u)
        return 0u;

    const auto resource = state.words[23u];
    std::array<std::uint32_t, 3> position{};
    std::array<std::uint32_t, 3> scale{};
    for (std::uint32_t i = 0u; i != 3u; ++i) {
        position[i] = runtime.ReadWord(resource + 112u + i * 4u);
    }
    std::array<float, 3> at{};
    for (std::uint32_t i = 0u; i != 3u; ++i)
        at[i] = std::bit_cast<float>(state.words[15u + i]) -
                std::bit_cast<float>(position[i]);
    (void)runtime.Normalize(at);

    std::array<float, 3> right{at[2u], 0.0f, -at[0u]};
    (void)runtime.Normalize(right);
    std::array<float, 3> up{
        right[2u] * at[1u] - right[1u] * at[2u],
        right[0u] * at[2u] - right[2u] * at[0u],
        right[1u] * at[0u] - right[0u] * at[1u]};
    const auto norm_result = runtime.Normalize(up);

    RtkPrtStdAtomicAimMatrix matrix{};
    matrix.right = right;
    matrix.up = up;
    matrix.at = at;
    runtime.UpdateMatrix(matrix);
    const auto frame = runtime.GetFrame(state);
    for (std::uint32_t i = 0u; i != 3u; ++i)
        scale[i] = runtime.ReadWord(resource + 100u + i * 4u);
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, scale, 0u);
    runtime.ApplyMatrix(frame, matrix, 2u);
    runtime.TranslateFrame(frame, position, 2u);

    const std::array<std::uint32_t, 3> offset{
        state.words[18u], state.words[19u], state.words[20u]};
    std::array<float, 3> angles{};
    runtime.UpdateFrameRotation(norm_result, frame, offset, angles);
    return runtime.SetRotation(state, angles, 0u);
}

}  // namespace pal4
