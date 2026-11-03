#include "pal4/dynamic_strip_revolute_frame.h"

namespace pal4 {
namespace {

constexpr std::array<std::array<float, 3>, 3> kAxes{{
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

std::uint32_t DynamicStripRevoluteFrame::Apply(
    DynamicStripObjectState& state,
    const std::array<float, 3>& target,
    const std::array<float, 3>& rotation,
    const std::array<float, 3>& step,
    IDynamicStripRevoluteFrameRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;

    const auto frame = runtime.GetFrame(state);
    const auto matrix = frame + 16u;
    const std::array<float, 3> negative_target{
        -target[0u], -target[1u], -target[2u]};
    runtime.TranslateMatrix(matrix, negative_target, 2u);
    for (std::size_t i = 0u; i < kAxes.size(); ++i)
        runtime.RotateMatrix(matrix, kAxes[i], rotation[i], 2u);
    runtime.TranslateMatrix(matrix, target, 2u);
    runtime.UpdateFrameObjects(frame);

    std::array<float, 3> values{};
    for (std::uint32_t i = 0u; i < values.size(); ++i)
        values[i] = Approach(runtime.ReadFloat(frame + 64u + 4u * i),
                             target[i], step[i]);
    return runtime.SetPosition(state, values, 0u);
}

}  // namespace pal4
