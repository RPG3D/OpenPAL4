#include "pal4/combat_console.h"

#include <algorithm>
#include <vector>

namespace pal4 {
namespace {

void SelectActorOfType(const std::int32_t type,
                       ICombatConsoleEventRuntime& runtime) {
    auto actors = runtime.RoleActors();
    while (!actors->AtEnd()) {
        const auto actor = actors->Current();
        if (runtime.ActorOrder(actor) == type) {
            runtime.DispatchActorEvent("re_substitute", actor, 1u);
            break;
        }
        actors->Advance();
    }
}

void CycleTarget(const std::uint32_t key,
                 ICombatConsoleEventRuntime& runtime) {
    const auto system = runtime.CombatSystem();
    const auto current = runtime.CurrentTarget(system);
    if (current == 0u) return;

    auto actors = runtime.TargetCandidates(current);
    std::vector<std::uint32_t> candidates;
    while (!actors->AtEnd()) {
        const auto actor = actors->Current();
        if (runtime.IsValidTarget(runtime.CombatSystem(), actor))
            candidates.push_back(actor);
        actors->Advance();
    }
    std::sort(candidates.begin(), candidates.end(),
              [&runtime](const std::uint32_t left,
                         const std::uint32_t right) {
                  return runtime.ActorOrder(left) <
                         runtime.ActorOrder(right);
              });
    for (std::size_t index = 0u; index < candidates.size(); ++index) {
        if (candidates[index] != current) continue;
        const auto next = key == 203u || key == 208u
                              ? (index + candidates.size() - 1u) %
                                    candidates.size()
                              : (index + 1u) % candidates.size();
        runtime.SelectTarget(runtime.CombatSystem(), candidates[next]);
    }
}

}  // namespace

bool CombatConsoleWindow::ProcessCombatEvent(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleEventRuntime& runtime) {
    const auto key = runtime.ReadWord(event + 16u);
    if (key == 19u)
        runtime.DispatchInputEvent("repeat_action", event, 1u);
    if (key == 28u)
        (void)DispatchVisibleCombatResult(console, runtime.ResultRuntime());
    if (key >= 59u && key <= 61u)
        SelectActorOfType(static_cast<std::int32_t>(key - 59u), runtime);

    switch (key) {
    case 1u:
        runtime.StartCombat(runtime.CombatSystem());
        return true;
    case 28u:
        runtime.ConfirmCombat(runtime.CombatSystem());
        return true;
    case 200u:
    case 203u:
    case 205u:
    case 208u:
        CycleTarget(key, runtime);
        return true;
    default:
        return false;
    }
}

}  // namespace pal4
