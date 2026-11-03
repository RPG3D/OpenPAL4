#include "pal4/monster_combat_position_dispatcher.h"

#include "pal4/monster_behavior_manager.h"

namespace pal4 {

std::int32_t MonsterCombatPositionRuntimeStub::PositionCount(
    std::uint32_t) { return 0; }
std::uint32_t MonsterCombatPositionRuntimeStub::PositionId(
    std::uint32_t, std::uint32_t) { return 0u; }
std::int32_t MonsterCombatPositionRuntimeStub::PositionValue(
    std::uint32_t, const std::string&, std::int32_t fallback) {
    return fallback;
}
void MonsterCombatPositionRuntimeStub::AppendRecord(
    MonsterBehaviorManagerState&, std::uint32_t, std::uint32_t,
    std::uint32_t, const std::array<std::uint32_t, 3>&) {}
void MonsterCombatPositionRuntimeStub::AddManagerText(
    std::uint32_t, std::uint32_t) {}
IMonsterActionDispatchRuntime&
MonsterCombatPositionRuntimeStub::ActionRuntime() { return action_; }

std::uint32_t MonsterCombatPositionDispatcher::Process(
    MonsterBehaviorManagerState& state, std::uint32_t manager_address,
    IMonsterCombatPositionRuntime& runtime) {
    for (std::int32_t index = 0;
         index < runtime.PositionCount(state.words[0x654u / 4u]); ++index) {
        const auto source = state.words[0x654u / 4u];
        const auto key = std::string("AAB_MST_ESSBEH_combat_pos_") +
                         std::to_string(index);
        const std::array<std::uint32_t, 3> record{
            runtime.PositionId(source, static_cast<std::uint32_t>(index)),
            static_cast<std::uint32_t>(
                runtime.PositionValue(source, key, 0)),
            state.words[0x730u / 4u]};
        runtime.AppendRecord(state, manager_address + 0x710u,
                             state.words[0x718u / 4u], 1u, record);
    }
    runtime.AddManagerText(manager_address, state.words[1u]);
    auto& action = runtime.ActionRuntime();
    return action.IterateAndCallback(
        action.ManagerInstance(), kCallback, manager_address);
}

}  // namespace pal4
