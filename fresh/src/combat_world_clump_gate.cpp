#include "pal4/combat_world_clump_gate.h"

namespace pal4 {

bool CombatWorldClumpGate::Add(
    const std::uint32_t clump, ICombatWorldClumpRuntime& runtime) {
    return runtime.AddClump(runtime.CombatSystem() + 116u, clump) > 0;
}

}  // namespace pal4
