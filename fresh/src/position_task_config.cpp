#include "pal4/position_task.h"

#include "position_task_state.h"

#include <array>
#include <string_view>

namespace pal4 {
namespace {
constexpr std::array<std::string_view, 3u> kPositionKeys{
    "Attr_Task_Position_x", "Attr_Task_Position_y", "Attr_Task_Position_z"};
constexpr std::array<std::string_view, 3u> kRotationKeys{
    "Attr_Task_Position_Rot_Fixed_x", "Attr_Task_Position_Rot_Fixed_y",
    "Attr_Task_Position_Rot_Fixed_z"};
constexpr std::array<std::string_view, 3u> kPositionBiasKeys{
    "Attr_Task_Position_Pos_Bias_x", "Attr_Task_Position_Pos_Bias_y",
    "Attr_Task_Position_Pos_Bias_z"};
constexpr std::array<std::string_view, 3u> kRotationBiasKeys{
    "Attr_Task_Position_Rot_Bias_x", "Attr_Task_Position_Rot_Bias_y",
    "Attr_Task_Position_Rot_Bias_z"};
constexpr std::array<std::string_view, 3u> kAddedVelocityKeys{
    "Attr_Add_Vel_x", "Attr_Add_Vel_y", "Attr_Add_Vel_z"};

PositionVector ReadVector(IPositionTaskConfigRuntime& runtime,
                          std::uint32_t config,
                          const std::array<std::string_view, 3u>& keys) {
    PositionVector result{};
    for (std::size_t i = 0u; i < result.size(); ++i)
        result[i] = runtime.ReadFloat(config, keys[i], 0.0f);
    return result;
}
}  // namespace

float PositionTask::ReadOptionalFloat(IPositionTaskConfigRuntime& runtime,
                                      std::uint32_t config,
                                      std::string_view key, float fallback) {
    return runtime.ReadFloat(config, key, fallback);
}

bool PositionTask::LoadConfig(PositionTaskState& state, std::uint32_t config,
                              IPositionTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    using namespace position_task_detail;
    state.words[21u] = 0u;
    WriteByte(state, 72u, runtime.ReadInt(
        config, "Attr_Task_Position_Result", 0) > 0);
    WriteVector(state, 88u, ReadVector(runtime, config, kPositionKeys));
    WriteByte(state, 136u, runtime.ReadInt(
        config, "Attr_Task_Position_Face_Target", 0) > 0);
    WriteVector(state, 140u, ReadVector(runtime, config, kRotationKeys));
    const auto position_bias = ReadVector(runtime, config, kPositionBiasKeys);
    const auto rotation_bias = ReadVector(runtime, config, kRotationBiasKeys);
    if (ReadByte(state, 72u) != 0u) {
        auto position = ReadVector(state, 88u);
        for (std::size_t i = 0u; i < position.size(); ++i)
            position[i] += runtime.RandomFloat(
                -position_bias[i], position_bias[i]);
        WriteVector(state, 88u, position);
    }
    auto rotation = ReadVector(state, 140u);
    for (std::size_t i = 0u; i < rotation.size(); ++i)
        rotation[i] += runtime.RandomFloat(
            -rotation_bias[i], rotation_bias[i]);
    WriteVector(state, 140u, rotation);
    WriteByte(state, 61u, runtime.ReadInt(
        config, "Attr_Inherit_Pre_Task", 0) > 0);
    WriteByte(state, 73u, runtime.ReadInt(
        config, "Attr_Add_Vel", 0) > 0);
    WriteVector(state, 204u, ReadVector(runtime, config, kAddedVelocityKeys));
    WriteByte(state, 188u, runtime.ReadInt(
        config, "Attr_Task_Position_Rollback", 0) > 0);
    WriteFloat(state, 80u, ReadOptionalFloat(runtime, config,
        "Attr_Task_Position_Time_Bias", 0.0f));
    WriteFloat(state, 168u, ReadOptionalFloat(runtime, config,
        "Attr_Time_Interval", 0.0f));
    WriteFloat(state, 180u, ReadOptionalFloat(runtime, config,
        "Attr_Time_Interval2", 0.0f));
    return true;
}

}  // namespace pal4
