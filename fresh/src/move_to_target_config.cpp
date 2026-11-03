#include "pal4/move_to_target_task.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 6u> kBaseKeys{
    "Attr_Task_Target_Position_x", "Attr_Task_Target_Position_y",
    "Attr_Task_Target_Position_z", "Attr_Task_Target_Rotate_x",
    "Attr_Task_Target_Rotate_y", "Attr_Task_Target_Rotate_z"};
constexpr std::array<std::string_view, 6u> kBiasKeys{
    "Attr_Task_Target_Position_Bias_x", "Attr_Task_Target_Position_Bias_y",
    "Attr_Task_Target_Position_Bias_z", "Attr_Task_Target_Rotate_Bias_x",
    "Attr_Task_Target_Rotate_Bias_y", "Attr_Task_Target_Rotate_Bias_z"};

float BiasedValue(const float base, const float bias,
                  IMoveToTargetConfigRuntime& runtime) {
    const auto random = static_cast<double>(runtime.RandomInt());
    const auto spread = static_cast<double>(bias);
    return static_cast<float>(static_cast<double>(base) - spread +
                              random * (2.0 * spread) * 0.000030518509);
}

}  // namespace

bool MoveToTargetTask::LoadConfig(
    MoveToTargetTaskState& state, const std::uint32_t config,
    IMoveToTargetConfigRuntime& runtime) {
    if (config == 0u)
        return false;

    auto target_name = runtime.ReadTargetName(
        config, "Attr_Task_Target_Name", 0x8e425cu);
    try {
        runtime.AssignTargetName(state, target_name);
    } catch (...) {
        runtime.ReleaseTargetName(target_name);
        throw;
    }
    runtime.ReleaseTargetName(target_name);

    for (std::size_t index = 0u; index < kBaseKeys.size(); ++index)
        state.words[21u + index] = std::bit_cast<std::uint32_t>(
            runtime.ReadFloat(config, kBaseKeys[index], 0.0f));

    // Native order reads one group of three bias keys, then consumes three
    // CRT random values before reading the next group.
    for (std::size_t group = 0u; group < 2u; ++group) {
        std::array<float, 3u> biases{};
        for (std::size_t axis = 0u; axis < 3u; ++axis)
            biases[axis] = runtime.ReadFloat(
                config, kBiasKeys[group * 3u + axis], 0.0f);
        for (std::size_t axis = 0u; axis < 3u; ++axis) {
            const auto word = 21u + group * 3u + axis;
            state.words[word] = std::bit_cast<std::uint32_t>(BiasedValue(
                std::bit_cast<float>(state.words[word]), biases[axis], runtime));
        }
    }

    const auto fixed = runtime.ReadTreeInt(config, "Attr_Is_Fixed_Vel", 0);
    state.words[34u] = (state.words[34u] & 0xffffff00u) |
                       static_cast<std::uint32_t>(fixed > 0);
    state.words[35u] = std::bit_cast<std::uint32_t>(runtime.ReadTreeFloat(
        config, "Attr_Fixed_Vel", 0.0f));
    return true;
}

}  // namespace pal4
