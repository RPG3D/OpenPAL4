#include "pal4/combat_random_condition_response.h"

#include <bit>

namespace pal4 {
namespace {

bool PassesRandomGate(std::uint32_t action,
                      ICombatRandomConditionResponseRuntime& runtime) {
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    const auto threshold = std::bit_cast<float>(
        runtime.ReadWord(action + 20u));
    return !(roll > threshold);
}

}  // namespace

std::int32_t CombatRandomConditionResponse::Process(
    std::uint32_t action, std::uint32_t context, std::uint32_t target,
    ICombatRandomConditionResponseRuntime& runtime) {
    if (!PassesRandomGate(action, runtime)) return 0;

    if (runtime.CheckCondition(target + 0x154u, 1u))
        return runtime.ApplyEvent(target, context, action, 1u);

    const auto stat = runtime.ActorStat(target, 7u);
    const auto negative = std::bit_cast<std::int32_t>(
        0u - std::bit_cast<std::uint32_t>(stat));
    return runtime.AdjustStat(target, 7u, negative);
}

std::int32_t CombatRandomConditionResponse::Trigger(
    std::uint32_t action, std::uint32_t target,
    ICombatRandomConditionResponseRuntime& runtime) {
    if (!PassesRandomGate(action, runtime)) return 0;
    return runtime.TriggerEvent(target);
}

}  // namespace pal4
