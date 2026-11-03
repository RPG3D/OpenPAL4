#pragma once
#include <cstdint>
namespace pal4 {
class NativeCombatPlayerManager;class NativeCombatMonsterManager;
//Original529B70: both first rosters' actor728 queues. Does not render models,
//effects, UI or CombatConsole; preserve their enclosing576220 phase ordering.
void DispatchNativeCombatRenderables(std::uint32_t context,NativeCombatPlayerManager&,NativeCombatMonsterManager&);
}
