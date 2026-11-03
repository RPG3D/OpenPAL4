#include "pal4/effect_object_revolute_frame.h"

namespace pal4 {
namespace {

constexpr std::array<std::array<float, 3u>, 3u> kAxes{{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}}};

float Approach(const float current, const float target, const float step) {
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

std::uint32_t EffectObjectRevoluteFrame::Apply(
    EffectObjectState& state, const std::array<float, 3u>& target,
    const std::array<float, 3u>& rotation,
    const std::array<float, 3u>& step, const std::uint32_t entry_eax,
    IEffectObjectRevoluteFrameRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return entry_eax & 0xffffff00u;

    const auto frame = runtime.GetFrame(state);
    const auto matrix = frame + 16u;
    const std::array<float, 3u> negative_target{
        -target[0u], -target[1u], -target[2u]};
    runtime.TranslateMatrix(matrix, negative_target, 2u);
    for (std::uint32_t i = 0u; i != 3u; ++i)
        runtime.RotateMatrix(matrix, kAxes[i], rotation[i], 2u);
    runtime.TranslateMatrix(matrix, target, 2u);
    runtime.UpdateFrameObjects(frame);

    std::array<float, 3u> next{};
    for (std::uint32_t i = 0u; i != 3u; ++i)
        next[i] = Approach(runtime.ReadFloat(frame + 64u + 4u * i),
                           target[i], step[i]);
    return runtime.SetPosition(state, next, 0u);
}

}  // namespace pal4
