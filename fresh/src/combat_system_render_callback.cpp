#include "pal4/combat_system_render_callback.h"

namespace pal4 {

std::uint32_t CombatSystemRenderCallback::Invoke(
    std::uint32_t argument,
    ICombatSystemRenderCallbackRuntime& runtime) {
    const auto combat_system = runtime.GetCombatSystem();
    return runtime.ProcessEvent61(combat_system, argument);
}

std::uint32_t CombatSystemRenderCallback::ProcessEvent61(
    const std::uint32_t combat_system, const std::uint32_t argument,
    ICombatSystemRenderEventRuntime& runtime) {
    if (runtime.ReadByte(combat_system + 516u) != 0u) return 2u;
    return runtime.ForwardEvent61(combat_system, argument);
}

}  // namespace pal4
