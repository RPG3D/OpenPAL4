#include "pal4/combat_event_property_dispatch.h"

#include <bit>

namespace pal4 {
namespace {

std::int32_t Dispatch(std::uint32_t action, std::uint32_t target,
                      std::uint32_t scale_bits,
                      ICombatEventPropertyDispatchRuntime& runtime) {
    std::int32_t result = 0;
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        const auto value = std::bit_cast<float>(
            runtime.ReadWord(action + 20u + index * 4u));
        const auto id = runtime.FloatToInt(value);
        const auto event = runtime.LookupEvent(id);
        result = std::bit_cast<std::int32_t>(event);
        if (event != 0u)
            result = runtime.ApplyEvent(event, target, scale_bits);
    }
    return result;
}

}  // namespace

std::int32_t CombatEventPropertyDispatch::Process(
    std::uint32_t action, std::uint32_t target,
    ICombatEventPropertyDispatchRuntime& runtime) {
    return Dispatch(action, target, std::bit_cast<std::uint32_t>(1.0f), runtime);
}

std::int32_t CombatEventPropertyDispatch::Clear(
    std::uint32_t action, std::uint32_t target,
    ICombatEventPropertyDispatchRuntime& runtime) {
    return Dispatch(action, target, 0u, runtime);
}

}  // namespace pal4
