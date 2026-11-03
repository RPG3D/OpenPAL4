#include "pal4/rtk_prt_std_atomic_revolute_frame.h"

namespace pal4 {
namespace {

constexpr std::array<std::array<float, 3>, 3> kAxes{{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}}};

float Approach(float current, float target, float step) {
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

std::uint32_t RtkPrtStdAtomicRevoluteFrame::Apply(
    RtkPrtStdAtomicObjectState& state,
    const std::array<float, 3>& target,
    const std::array<float, 3>& rotation,
    const std::array<float, 3>& step,
    const std::uint32_t entry_eax,
    IRtkPrtStdAtomicRevoluteRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return entry_eax & 0xffffff00u;

    const auto frame = runtime.GetFrame(state);
    const auto matrix = frame + 16u;
    const std::array<float, 3> negative{
        -target[0u], -target[1u], -target[2u]};
    runtime.TranslateMatrix(matrix, negative, 2u);
    for (std::uint32_t i = 0u; i != 3u; ++i)
        runtime.RotateMatrix(matrix, kAxes[i], rotation[i], 2u);
    runtime.TranslateMatrix(matrix, target, 2u);
    runtime.UpdateFrameObjects(frame);

    std::array<float, 3> next{};
    for (std::uint32_t i = 0u; i != 3u; ++i)
        next[i] = Approach(runtime.ReadFloat(frame + 64u + i * 4u),
                           target[i], step[i]);
    return runtime.SetPosition(state, next, 0u);
}

}  // namespace pal4
