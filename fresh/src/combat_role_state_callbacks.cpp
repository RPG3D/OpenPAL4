#include "pal4/combat_role_state_callbacks.h"

namespace pal4 {

std::uint8_t CombatRoleStateCallbacks::OnMouseClick(
    const std::uint32_t event, ICombatRoleStateCallbackRuntime& runtime) {
    (void)event;
    const std::array<std::uint32_t, 2> payload{};
    runtime.DispatchUiEvent(runtime.UiFrameManager() + 260u,
                            kUiEventKeyAddress, payload);
    return 1u;
}

std::uint8_t CombatRoleStateCallbacks::OnSubstitutionClick(
    const std::uint32_t event, ICombatRoleStateCallbackRuntime& runtime) {
    const auto control = runtime.ReadWord(event + 8u);
    const auto actor = runtime.ReadWord(control + 280u);
    if (!actor) return 0u;
    const auto key = runtime.ReadWord(kSubstituteKeyAddress);
    const auto router = runtime.ScriptEventRouter();
    runtime.DispatchScriptEvent(router, key, actor + 716u, 1u);
    return 1u;
}

}  // namespace pal4
