#include "pal4/combat_conditional_event_dispatch.h"

#include <bit>

namespace pal4 {

void CombatConditionalEventDispatch::Process(
    std::uint32_t action, std::uint32_t target, std::uint32_t context,
    ICombatConditionalEventDispatchRuntime& runtime) {
    const auto check = std::bit_cast<float>(runtime.ReadWord(action + 20u));
    const auto id = runtime.FloatToInt(check);
    const auto selected = std::bit_cast<float>(runtime.ReadWord(
        action + (runtime.ElementExists(context, id) ? 24u : 28u)));
    if (selected == 0.0f) return;
    runtime.Dispatch(target, runtime.FloatToInt(selected), context);
}

}  // namespace pal4
