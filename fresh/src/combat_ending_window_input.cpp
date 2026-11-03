#include "pal4/combat_ending_window_input.h"

namespace pal4 {

std::uint32_t CombatEndingWindowInput::FinishCombat(
    const std::uint32_t window,
    ICombatEndingWindowInputRuntime& runtime) {
    const auto system = runtime.CombatSystem();
    const auto result = runtime.EndCombatSequence(
        system, runtime.ReadWord(window + 436u));
    runtime.WriteWord(window + 436u, 3u);
    return result;
}

std::uint8_t CombatEndingWindowInput::HandleInput(
    const std::uint32_t navigation, const std::uint32_t event,
    ICombatEndingWindowInputRuntime& runtime) {
    const auto window = navigation - 40u;
    if (runtime.ReadWord(window + 436u) == 3u) return 0u;
    const auto key = runtime.ReadWord(event + 16u);
    if (key == 1u) {
        (void)FinishCombat(window, runtime);
        return 1u;
    }
    if (key != 28u) return 0u;
    const auto result_state = runtime.ReadWord(window + 716u);
    if (result_state == 0u)
        runtime.ShowResult(window);
    else if (result_state == 1u)
        runtime.HideResult(window);
    else
        (void)FinishCombat(window, runtime);
    return 1u;
}

std::uint8_t CombatEndingWindowInput::HandleMouseDown(
    const std::uint32_t window, const std::uint32_t event,
    ICombatEndingWindowInputRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 0u) {
        const auto result_state = runtime.ReadWord(window + 716u);
        if (result_state == 0u)
            runtime.ShowResult(window);
        else if (result_state == 1u)
            runtime.HideResult(window);
        else
            (void)FinishCombat(window, runtime);
    }
    return 1u;
}

}  // namespace pal4
