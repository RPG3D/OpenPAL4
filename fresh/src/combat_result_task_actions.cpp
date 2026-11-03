#include "pal4/combat_result_task_actions.h"

namespace pal4 {

std::uint8_t CombatResultTaskActions::SetState(
    const std::uint32_t element, ICombatResultTaskRuntime& runtime) {
    const auto dialog = runtime.ReadWord(element + 60u);
    const auto state = runtime.ReadWord(element + 64u);
    runtime.WriteWord(dialog + 716u, state);
    return 1u;
}

std::uint8_t CombatResultTaskActions::ShowPanel(
    const std::uint32_t element, ICombatResultTaskRuntime& runtime) {
    runtime.PlaySound("WA46.wav", 1u);
    const auto dialog = runtime.ReadWord(element + 60u);
    const auto panel = runtime.ReadWord(dialog + 88u);
    runtime.SetVisible(panel, true);
    return 1u;
}

std::uint8_t CombatResultTaskActions::FinishCombat(
    const std::uint32_t element, ICombatResultTaskRuntime& runtime) {
    const auto result = runtime.ReadWord(element + 60u);
    runtime.EndCombatSequence(result);
    return 1u;
}

}  // namespace pal4
