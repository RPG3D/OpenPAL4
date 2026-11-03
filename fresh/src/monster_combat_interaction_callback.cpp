#include "pal4/monster_combat_interaction_callback.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float Position(const MonsterBehaviorManagerState& state,
               std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

}  // namespace

MonsterCombatCandidate MonsterCombatInteractionRuntimeStub::Candidate(
    std::uint32_t) { return {}; }
std::string MonsterCombatInteractionRuntimeStub::ScriptFunction(
    std::uint32_t) { return {}; }
double MonsterCombatInteractionRuntimeStub::TargetDoubleProperty(
    std::uint32_t) { return 0.0; }
double MonsterCombatInteractionRuntimeStub::PlanarDistance(
    float dx, float dz) {
    const float squared = dx * dx + dz * dz;
    return std::sqrt(static_cast<double>(squared));
}
float MonsterCombatInteractionRuntimeStub::InteractionRadius(
    std::uint32_t) { return 0.0f; }
std::uint32_t MonsterCombatInteractionRuntimeStub::CollisionClump() {
    return 0u;
}
bool MonsterCombatInteractionRuntimeStub::CollisionBlocked(
    const std::array<float, 6>&, std::uint32_t) { return false; }
IMonsterCombatPositionRuntime& MonsterCombatInteractionRuntimeStub::Records() {
    return records_;
}

std::uint32_t MonsterCombatInteractionCallback::Process(
    MonsterBehaviorManagerState& manager_state,
    std::uint32_t candidate_address, std::uint32_t manager_address,
    IMonsterCombatInteractionRuntime& runtime) {
    const auto target_address = manager_state.words[0x6e4u / 4u];
    if (target_address == 0u || candidate_address == manager_address)
        return candidate_address;

    const auto candidate = runtime.Candidate(candidate_address);
    if (!candidate.active || candidate.pursuit_mode != 1u)
        return candidate_address;
    if (candidate.source_address != 0u &&
        !runtime.ScriptFunction(candidate.source_address).empty())
        return candidate_address;

    const auto height_difference = std::fabs(
        static_cast<double>(Position(manager_state, 0xb0u)) -
        candidate.position[1]);
    if (runtime.TargetDoubleProperty(target_address) < height_difference)
        return candidate_address;

    const auto dx = candidate.position[0] - Position(manager_state, 0xacu);
    const auto dz = candidate.position[2] - Position(manager_state, 0xb4u);
    const auto distance = runtime.PlanarDistance(dx, dz);
    const auto radius = runtime.InteractionRadius(
        manager_state.words[0x654u / 4u]);
    if (!(distance <= static_cast<double>(radius)))
        return candidate_address;

    const std::array<float, 6> endpoints{
        Position(manager_state, 0xe0u),
        Position(manager_state, 0xe4u),
        Position(manager_state, 0xe8u),
        candidate.collision_position[0],
        candidate.collision_position[1],
        candidate.collision_position[2]};
    if (runtime.CollisionBlocked(endpoints, runtime.CollisionClump()))
        return candidate_address;

    auto& records = runtime.Records();
    for (std::int32_t index = 0;
         index < records.PositionCount(candidate.source_address); ++index) {
        const auto key = std::string("AAB_MST_ESSBEH_combat_pos_") +
                         std::to_string(index);
        const std::array<std::uint32_t, 3> record{
            records.PositionId(candidate.source_address,
                               static_cast<std::uint32_t>(index)),
            static_cast<std::uint32_t>(records.PositionValue(
                candidate.source_address, key, 0)),
            candidate.timer_word};
        records.AppendRecord(manager_state, manager_address + 0x710u,
                             manager_state.words[0x718u / 4u], 1u, record);
    }
    records.AddManagerText(manager_address, candidate.text_address);
    return candidate_address;
}

}  // namespace pal4
