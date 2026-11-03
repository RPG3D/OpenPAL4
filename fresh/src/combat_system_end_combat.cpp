#include "pal4/combat_system_end_combat.h"

namespace pal4 {

std::uint32_t CombatSystemEndCombat::EndWrapper(
    const std::uint32_t object, ICombatSystemEndRuntime& runtime) {
    return End(object, runtime);
}

std::uint32_t CombatSystemEndCombat::End(
    const std::uint32_t object, ICombatSystemEndRuntime& runtime) {
    runtime.WriteWord(object + 388u, 0u);
    runtime.WriteWord(object + 392u, 0u);
    runtime.WriteWord(object + 508u, 7u);
    runtime.DispatchActorEnd(object);
    runtime.DeinitializeGameObject(runtime.ReadWord(object + 424u));
    runtime.ClearCombatRosters();
    runtime.ResetConsole(runtime.GetConsole());
    runtime.SetRenderCallback(runtime.ReadWord(object + 428u));
    runtime.WriteByte(object + 516u, 1u);
    runtime.InitializeStrings(object + 432u);
    return 1u;
}

}  // namespace pal4
