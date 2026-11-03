#include "pal4/combat_probability_outcome.h"

#include <bit>
#include <cmath>

namespace pal4 {

std::int32_t CombatProbabilityOutcome::Process(
    std::uint32_t record, [[maybe_unused]] std::uint32_t unused,
    std::uint32_t context, std::uint32_t target,
    ICombatProbabilityOutcomeRuntime& runtime) {
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    const auto threshold = std::bit_cast<float>(
        runtime.ReadWord(record + 20u));
    if (roll > threshold) return 0;

    std::int32_t result = 0;
    for (std::uint32_t index = 0; index < 8u; ++index) {
        const auto value = std::bit_cast<float>(
            runtime.ReadWord(record + 24u + index * 4u));
        result = runtime.FloatToInt(value);
        if (result != 0)
            result = runtime.ApplyOutcome(target, result, context);
    }
    return result;
}

std::int32_t CombatProbabilityOutcomeExtended::Process(
    std::uint32_t record, [[maybe_unused]] std::uint32_t unused,
    std::uint32_t target, std::uint32_t actor,
    ICombatProbabilityOutcomeExtendedRuntime& runtime) {
    const auto index = runtime.FloatToInt(std::bit_cast<float>(
        runtime.ReadWord(record + 24u)));
    if (index < 39 || index > 47) return index;

    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    const auto threshold = std::bit_cast<float>(runtime.ReadWord(record + 20u));
    if (roll > threshold) return 0;

    const auto event = runtime.LookupEvent(index);
    if (event == 0u) return 0;

    const auto current = runtime.FloatToInt(
        runtime.QueryEventValue(event, actor + 0x154u));
    const auto expected = runtime.FloatToInt(std::bit_cast<float>(
        runtime.ReadWord(record + 28u)));
    if (current != expected) return expected;

    const auto value = runtime.FloatToInt(std::bit_cast<float>(
        runtime.ReadWord(record + 32u)));
    return runtime.ApplyOutcome(target, value, actor);
}

}  // namespace pal4
