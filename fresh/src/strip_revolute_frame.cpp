#include "pal4/strip_revolute_frame.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::array<float, 3>, 3> kAxes{{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}}};

float Approach(const float current, const float target,
               const float step) {
    if (current > target) {
        const auto next = static_cast<double>(current) - step;
        return next < target ? target : static_cast<float>(next);
    }
    if (current < target) {
        const auto next = static_cast<double>(current) + step;
        return next > target ? target : static_cast<float>(next);
    }
    return current;
}

}  // namespace

std::uint32_t StripRevoluteFrame::Apply(
    StripObjectState& state, const std::array<float, 3>& pivot,
    const std::array<float, 3>& rotation,
    const std::array<float, 3>& step, const std::uint32_t entry_eax,
    IStripRevoluteRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return entry_eax & 0xffffff00u;

    const auto frame = runtime.GetFrame(state);
    const auto matrix = frame + 16u;
    runtime.TranslateMatrix(matrix,
                            {-pivot[0u], -pivot[1u], -pivot[2u]}, 2u);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        runtime.RotateMatrix(matrix, kAxes[i], rotation[i], 2u);
    runtime.TranslateMatrix(matrix, pivot, 2u);
    runtime.UpdateFrameObjects(frame);

    std::array<float, 3> next{};
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        const auto current = runtime.ReadFloat(frame + 64u + i * 4u);
        next[i] = Approach(current, pivot[i], step[i]);
    }
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[22u + i] = std::bit_cast<std::uint32_t>(next[i]);
    return runtime.RefreshPosition(state, {0.0f, 0.0f, 0.0f}, 1u);
}

}  // namespace pal4
