#include "pal4/combat_result_move.h"

namespace pal4 {

std::uint8_t CombatResultMove::Update(
    const std::uint32_t element, const float elapsed,
    ICombatResultMoveRuntime& runtime) {
    const auto initial_window = runtime.ReadWord(element + 60u);
    const auto next = runtime.GetYPosition(initial_window) + elapsed * 2.5f;
    const auto update_window = runtime.ReadWord(element + 60u);
    runtime.SetYPosition(update_window, next);
    const auto check_window = runtime.ReadWord(element + 60u);
    const auto position = runtime.GetYPosition(check_window);
    const auto target = runtime.ReadFloat(element + 64u);
    if (position > target) {
        const auto final_target = runtime.ReadFloat(element + 64u);
        const auto clamp_window = runtime.ReadWord(element + 60u);
        runtime.SetYPosition(clamp_window, final_target);
    }
    return 1u;
}

bool CombatResultMove::Reached(
    const std::uint32_t element, ICombatResultMoveRuntime& runtime) {
    const auto window = runtime.ReadWord(element + 60u);
    const auto position = runtime.GetYPosition(window);
    return position >= runtime.ReadFloat(element + 64u);
}

}  // namespace pal4
