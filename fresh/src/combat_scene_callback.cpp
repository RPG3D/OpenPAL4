#include "pal4/combat_scene_callback.h"

namespace pal4 {

std::uint32_t CombatSceneCallback::Clone(
    std::uint32_t source, ICombatSceneRuntime& runtime) {
    const auto clone = runtime.Allocate(12u);
    if (clone == 0u) return 0u;
    runtime.WriteWord(clone, kBaseVtable);
    runtime.WriteWord(clone + 4u, runtime.ReadWord(source + 4u));
    runtime.WriteWord(clone + 8u, runtime.ReadWord(source + 8u));
    runtime.WriteWord(clone, kVtable);
    return clone;
}

}  // namespace pal4
