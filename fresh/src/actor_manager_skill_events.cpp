#include "pal4/actor_manager_skill_events.h"

#include <array>

namespace pal4 {

std::uint32_t ActorManagerSkillEvents::TriggerSkill(
    std::uint32_t actor, std::uint32_t first, std::uint32_t second,
    std::uint32_t third, std::uint32_t fourth,
    IActorManagerSkillEventRuntime& runtime) {
    const std::array payload{actor + 716u, first, fourth, third, second};
    return runtime.Dispatch("ae_triger_skill", payload, 1u);
}

void ActorManagerSkillEvents::EmptyFourArgumentCallback(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t ActorManagerSkillEvents::CallFunctionId(
    std::uint32_t actor, std::uint32_t first, std::uint32_t second,
    std::uint32_t third, IActorManagerSkillEventRuntime& runtime) {
    const std::array payload{actor + 716u, first, third, second};
    return runtime.Dispatch("ae_call_fun_id", payload, 1u);
}

void ActorManagerSkillEvents::EmptyThreeArgumentCallback(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t ActorManagerSkillEvents::RemoveMutexSkill(
    std::uint32_t actor, std::uint32_t skill,
    IActorManagerSkillEventRuntime& runtime) {
    runtime.WriteByte(actor + 892u, 1u);
    const std::array payload{actor + 716u, skill};
    return runtime.Dispatch("ae_remove_mutex_skill", payload, 1u);
}

}  // namespace pal4
