#include "pal4/combat_event_list_dispatch.h"

#include <bit>

namespace pal4 {

std::int32_t CombatEventListDispatch::Process(
    std::uint32_t action, std::uint32_t target,
    ICombatEventListDispatchRuntime& runtime) {
    std::int32_t result = 0;
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        const auto value = std::bit_cast<float>(
            runtime.ReadWord(action + 20u + index * 4u));
        const auto id = runtime.FloatToInt(value);
        result = runtime.DispatchId(target, id);
    }
    return result;
}

std::int32_t CombatEventListDispatch::ProcessSecond(
    std::uint32_t action, std::uint32_t target,
    ICombatEventListDispatchRuntime& runtime) {
    return Process(action, target, runtime);
}

}  // namespace pal4
