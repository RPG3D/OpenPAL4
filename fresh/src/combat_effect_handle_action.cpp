#include "pal4/combat_effect_handle_action.h"

namespace pal4 {

void CombatEffectHandleAction::Process(
    std::uint32_t action, std::uint32_t target,
    ICombatEffectHandleActionRuntime& runtime) {
    CombatEffectHandle handle{};
    runtime.BuildHandle(target, handle, action + 0x64u, action + 0x74u);
    runtime.ResetHandle(handle);
}

}  // namespace pal4
