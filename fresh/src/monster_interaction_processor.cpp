#include "pal4/monster_interaction_processor.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>
#include <cmath>
#include <limits>

namespace pal4 {
namespace {

float Position(const MonsterBehaviorManagerState& state,
               std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

float Dot(const std::array<float, 3>& left,
          const std::array<float, 3>& right) {
    return static_cast<float>(
        static_cast<double>(left[0]) * right[0] +
        static_cast<double>(left[1]) * right[1] +
        static_cast<double>(left[2]) * right[2]);
}

void ClassifyFacing(MonsterBehaviorManagerState& state,
                    std::uint32_t manager_address,
                    std::uint32_t target_address,
                    const MonsterInteractionTarget& target,
                    IMonsterInteractionRuntime& runtime) {
    auto direction = std::array<float, 3>{
        target.position[0] - Position(state, 0xacu),
        target.position[1] - Position(state, 0xb0u),
        target.position[2] - Position(state, 0xb4u)};
    const auto target_forward = runtime.TargetForward(target_address);
    const auto manager_forward = runtime.ManagerForward(manager_address);
    runtime.NormalizeDirection(direction);

    const auto toward_target = Dot(manager_forward, direction);
    const auto forward_dot = Dot(manager_forward, target_forward);
    auto angle = runtime.AngleForForwardDot(forward_dot);
    if (manager_forward[0] < target_forward[0])
        angle = static_cast<double>(6.2831855f) - angle;
    const auto degrees = angle * static_cast<double>(57.29578f);
    if (degrees < 45.0 || degrees > 315.0)
        state.words[0x700u / 4u] = toward_target > 0.0f ? 1u : 2u;
}

}  // namespace

bool MonsterInteractionRuntimeStub::PlayerControlBusy() { return false; }
MonsterInteractionTarget MonsterInteractionRuntimeStub::Target(
    std::uint32_t) {
    const auto unavailable = std::numeric_limits<float>::quiet_NaN();
    return {{unavailable, unavailable, unavailable}};
}
double MonsterInteractionRuntimeStub::PlanarDistance(float dx, float dz) {
    const float squared = dx * dx + dz * dz;
    return std::sqrt(static_cast<double>(squared));
}
float MonsterInteractionRuntimeStub::PlayerDistanceBase() { return 0.0f; }
std::array<float, 3> MonsterInteractionRuntimeStub::TargetForward(
    std::uint32_t) { return {}; }
std::array<float, 3> MonsterInteractionRuntimeStub::ManagerForward(
    std::uint32_t) { return {}; }
void MonsterInteractionRuntimeStub::NormalizeDirection(
    std::array<float, 3>& direction) {
    const auto length = std::sqrt(static_cast<double>(direction[0]) * direction[0] +
                                  static_cast<double>(direction[1]) * direction[1] +
                                  static_cast<double>(direction[2]) * direction[2]);
    if (length != 0.0)
        for (auto& component : direction)
            component = static_cast<float>(component / length);
}
double MonsterInteractionRuntimeStub::AngleForForwardDot(float dot) {
    return std::acos(static_cast<double>(dot));
}
std::string MonsterInteractionRuntimeStub::ReadScriptName(
    std::uint32_t, const std::array<std::uint32_t, 4>&, const char*) { return {}; }
void MonsterInteractionRuntimeStub::InitializeScripts() {}
std::string MonsterInteractionRuntimeStub::ArenaScriptPath() { return {}; }
bool MonsterInteractionRuntimeStub::ScriptCached(const std::string&) {
    return false;
}
std::uint32_t MonsterInteractionRuntimeStub::CreateInterpreter(
    const std::string&, const std::string&, const std::string&) { return 0u; }
bool MonsterInteractionRuntimeStub::ExecuteInterpreter(std::uint32_t) {
    return false;
}
void MonsterInteractionRuntimeStub::HandleInterpreterError(std::uint32_t) {}
void MonsterInteractionRuntimeStub::ReleaseInterpreter(std::uint32_t) {}
void MonsterInteractionRuntimeStub::ShowMissingScriptError() {}
void MonsterInteractionRuntimeStub::CleanupScripts() {}
std::int32_t MonsterInteractionRuntimeStub::PendingStateCount(
    std::uint32_t) { return 1; }
void MonsterInteractionRuntimeStub::ReactivateManager(
    std::uint32_t, std::uint32_t) {}
void MonsterInteractionRuntimeStub::RestoreRenderCallback(std::uint32_t) {}

bool MonsterInteractionProcessor::Process(
    MonsterBehaviorManagerState& state, std::uint32_t manager_address,
    std::uint32_t target_address, IMonsterInteractionRuntime& runtime) {
    if (target_address == 0u || runtime.PlayerControlBusy())
        return false;
    const auto target = runtime.Target(target_address);
    const auto dx = target.position[0] - Position(state, 0xacu);
    const auto dz = target.position[2] - Position(state, 0xb4u);
    const auto distance = runtime.PlanarDistance(dx, dz);
    const auto limit = static_cast<double>(runtime.PlayerDistanceBase()) +
                       Position(state, 0xdcu);
    if (!(distance <= limit))
        return false;

    state.words[0x700u / 4u] = 0u;
    ClassifyFacing(state, manager_address, target_address, target, runtime);
    std::array<std::uint32_t, 4> key{};
    for (std::uint32_t index = 0u; index != key.size(); ++index)
        key[index] = state.words[(0xd0u + index * 4u) / 4u];
    const auto source = state.words[0x654u / 4u];
    const auto script_name = runtime.ReadScriptName(
        source, key, "AAB_MST_ESSBEH_script_func");
    if (script_name.empty())
        return true;

    runtime.InitializeScripts();
    const auto script_path = runtime.ArenaScriptPath();
    const auto full_path = script_path + "[" + script_name + "]";
    if (!runtime.ScriptCached(full_path)) {
        const auto interpreter = runtime.CreateInterpreter(
            script_path, full_path, script_name);
        if (interpreter != 0u && runtime.ExecuteInterpreter(interpreter)) {
            runtime.HandleInterpreterError(interpreter);
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowMissingScriptError();
        }
    }
    runtime.CleanupScripts();
    if (runtime.PendingStateCount(source) <= 0) {
        runtime.ReactivateManager(manager_address, 0x40000000u);
        runtime.RestoreRenderCallback(manager_address);
        return false;
    }
    return true;
}

}  // namespace pal4
