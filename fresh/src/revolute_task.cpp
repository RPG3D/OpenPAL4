#include "pal4/revolute_task.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(const RevoluteTaskState& state, const std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

void WriteFloat(RevoluteTaskState& state, const std::size_t word,
                const float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}

bool ReadFlag(const RevoluteTaskState& state, const std::size_t byte) {
    return ((state.words[byte / 4u] >> (8u * (byte % 4u))) & 0xffu) != 0u;
}

void WriteFlag(RevoluteTaskState& state, const std::size_t byte,
               const bool value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}

constexpr std::array<std::string_view, 9u> kRotationKeys{
    "Attr_Task_Revolute_OriginPos_x", "Attr_Task_Revolute_OriginPos_y",
    "Attr_Task_Revolute_OriginPos_z", "Attr_Task_Revolute_Angle_x",
    "Attr_Task_Revolute_Angle_y", "Attr_Task_Revolute_Angle_z",
    "Attr_Task_Revolute_Center_Vel_x", "Attr_Task_Revolute_Center_Vel_y",
    "Attr_Task_Revolute_Center_Vel_z"};

constexpr std::array<std::string_view, 3u> kAddVelocityKeys{
    "Attr_Add_Vel_x", "Attr_Add_Vel_y", "Attr_Add_Vel_z"};

std::array<float, 3u> Origin(const RevoluteTaskState& state) {
    return {ReadFloat(state, 22u), ReadFloat(state, 23u), ReadFloat(state, 24u)};
}

}  // namespace

RevoluteTaskState& RevoluteTask::Construct(RevoluteTaskState& state,
                                           IRevoluteTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    for (const auto word : {33u, 24u, 23u, 22u, 27u, 26u, 25u})
        state.words[word] = 0u;
    return state;
}

RevoluteTaskState& RevoluteTask::Destroy(RevoluteTaskState& state,
                                         IRevoluteTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view RevoluteTask::ClassName() { return "RevoluteTask"; }

bool RevoluteTask::LoadConfig(RevoluteTaskState& state,
                              const std::uint32_t config,
                              IRevoluteConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    for (std::size_t i = 0; i < kRotationKeys.size(); ++i)
        WriteFloat(state, 22u + i, runtime.ReadFloat(config, kRotationKeys[i], 0.0f));
    WriteFlag(state, 72u, runtime.ReadInt(config, "Attr_Add_Vel", 0) > 0);
    for (std::size_t i = 0; i < kAddVelocityKeys.size(); ++i)
        WriteFloat(state, 19u + i,
                   runtime.ReadFloat(config, kAddVelocityKeys[i], 0.0f));
    WriteFlag(state, 61u,
              runtime.ReadInt(config, "Attr_Inherit_Pre_Task", 0) > 0);
    WriteFlag(state, 73u,
              runtime.ReadInt(config, "Attr_Task_Revolute_Camera_Target", 0) > 0);
    WriteFlag(state, 124u,
              runtime.ReadInt(config, "Attr_Task_Revolute_Correct_Result", 0) > 0);
    return true;
}

std::uint8_t RevoluteTask::Start(RevoluteTaskState& state,
                                  IRevoluteTaskRuntime& runtime) {
    const auto owner = state.words[16u];
    state.words[33u] = runtime.ResolveFacet(owner, ClassName());
    if (ReadFlag(state, 72u)) {
        const auto reciprocal = 1.0f / ReadFloat(state, 5u);
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 19u + i,
                       (ReadFloat(state, 19u + i) - ReadFloat(state, 25u + i)) * reciprocal);
    }
    if (ReadFlag(state, 73u)) {
        const auto camera = runtime.ResolveFacet(owner, "StartCameraTask");
        if (camera != 0u && !runtime.CameraIsActive(camera) &&
            runtime.CameraHasTarget(camera)) {
            auto target = Origin(state);
            runtime.ReadCameraTarget(camera, target);
            for (std::size_t i = 0; i < 3u; ++i)
                WriteFloat(state, 22u + i, target[i]);
        }
    }
    state.words[32u] = 0u;
    return 1u;
}

std::uint8_t RevoluteTask::Update(RevoluteTaskState& state,
                                   const float elapsed,
                                   IRevoluteTaskRuntime& runtime) {
    if (ReadFlag(state, 72u)) {
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 25u + i,
                       elapsed * ReadFloat(state, 19u + i) + ReadFloat(state, 25u + i));
    }
    WriteFloat(state, 32u, elapsed + ReadFloat(state, 32u));
    std::array<float, 6u> motion{};
    for (std::size_t i = 0; i < motion.size(); ++i)
        motion[i] = elapsed * ReadFloat(state, 25u + i);
    runtime.ApplyRevolution(state.words[33u], Origin(state), motion);
    return 1u;
}

std::uint8_t RevoluteTask::StopAndCorrect(RevoluteTaskState& state,
                                           IRevoluteTaskRuntime& runtime) {
    const auto duration = ReadFloat(state, 5u);
    if (ReadFlag(state, 124u) && duration > 0.0f) {
        const auto remaining = duration - ReadFloat(state, 32u);
        const auto sign = remaining < 0.0f ? -1.0f : 1.0f;
        std::array<float, 6u> motion{};
        for (std::size_t i = 0; i < 3u; ++i) {
            motion[i] = (sign * ReadFloat(state, 19u + i) * remaining +
                         ReadFloat(state, 25u + i)) * remaining;
            motion[i + 3u] = sign * ReadFloat(state, 28u + i) * remaining;
        }
        runtime.ApplyRevolution(state.words[33u], Origin(state), motion);
    }
    state.words[33u] = 0u;
    state.words[32u] = 0u;
    return 1u;
}

}  // namespace pal4
