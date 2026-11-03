#include "pal4/combat_target_event_handler.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(std::uint32_t address, ICombatTargetEventRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

std::uint32_t ProcessMatchingEvent(std::uint32_t slot, std::int32_t id,
                                   std::uint32_t target,
                                   ICombatTargetEventRuntime& runtime) {
    const auto property = id == 2 ? 2u : 3u;
    const auto value = static_cast<float>(
        runtime.PropertyValue(target, property));
    runtime.ApplyProperty(slot, value);
    return runtime.ProcessObject(slot, target);
}

}  // namespace

std::uint8_t CombatTargetEventHandler::Process(
    std::uint32_t action, std::uint32_t target,
    ICombatTargetEventRuntime& runtime) {
    const auto type = runtime.TargetType(target);
    if (type == 4u) return 4u;

    const auto first_id = runtime.FloatToInt(
        ReadFloat(action + 32u, runtime));
    const auto second_id = runtime.FloatToInt(
        ReadFloat(action + 44u, runtime));
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    if (roll > ReadFloat(action + 20u, runtime)) return 0u;
    const auto state_three = runtime.IsStateThree(target);
    if (state_three != 0u) return state_three;

    const auto first_second = runtime.FloatToInt(
        ReadFloat(action + 40u, runtime));
    const auto first_scale = ReadFloat(action + 36u, runtime);
    runtime.SetEventData(0u, target, first_id, first_scale,
                         first_second);
    const auto second_second = runtime.FloatToInt(
        ReadFloat(action + 52u, runtime));
    const auto second_scale = ReadFloat(action + 48u, runtime);
    runtime.SetEventData(1u, target, second_id, second_scale,
                         second_second);

    if (first_id == 2 || first_id == 6)
        ProcessMatchingEvent(0u, first_id, target, runtime);
    if (second_id == 2 || second_id == 6) {
        return static_cast<std::uint8_t>(
            ProcessMatchingEvent(1u, second_id, target, runtime));
    }
    return static_cast<std::uint8_t>(second_id - 6);
}

}  // namespace pal4
