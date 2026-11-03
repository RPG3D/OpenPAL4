#include "pal4/actor_manager_skill_action.h"

namespace pal4 {

std::uint8_t ActorManagerSkillAction::Process(
    std::uint32_t actor, std::uint32_t skill_id,
    std::uint32_t target, IActorManagerSkillActionRuntime& runtime) {
    const auto skill = runtime.FindSkill(skill_id);
    if (skill == 0u) return 0u;

    if (skill_id >= 17006u && skill_id <= 17010u) {
        const auto index = skill_id - 17006u;
        if (runtime.CheckRandom(actor, index) ||
            runtime.CurrentState(actor) == 4u) {
            runtime.InvokeSpecial(actor, target, skill, index);
            return 0u;
        }
    }

    auto target_state = runtime.CaptureState(target);
    auto actor_state = runtime.CaptureState(actor);
    runtime.ProcessSkillChain(skill, target_state, actor_state);

    const auto chance = actor_state.FollowupChance();
    if (chance != 0u && runtime.CheckRandomExtended(actor, chance)) {
        runtime.InvokeFollowup(actor, target, skill, chance);
        return 0u;
    }
    return static_cast<std::uint8_t>(
        runtime.ProcessComplexList(actor, skill, target));
}

}  // namespace pal4
