#include "pal4/thunder_revolute_frame.h"

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

std::uint32_t ThunderRevoluteFrame::Apply(
    ThunderObjectState& state, const std::array<float, 3u>& pivot,
    const std::array<float, 3u>& rotation,
    const std::array<float, 3u>& step, const std::uint32_t entry_eax,
    IThunderRevoluteRuntime& runtime) {
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

    std::array<float, 3u> position{};
    for (std::uint32_t i = 0u; i < 3u; ++i)
        position[i] = Approach(runtime.ReadFloat(frame + 64u + 4u * i),
                               pivot[i], step[i]);
    return runtime.SetPosition(state, position, 0u);
}

}  // namespace pal4
