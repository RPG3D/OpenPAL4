#include "pal4/gob_obstacle_combat_callback.h"

namespace pal4 {

std::uint32_t GobObstacleCombatCallback::Construct(
    const std::uint32_t object,
    IGobObstacleCombatCallbackRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, 0x8493acu);
    runtime.WriteWord(object + 8u, 0u);
    return object;
}

std::uint32_t GobObstacleCombatCallback::ConstructBase(
    const std::uint32_t object,
    IGobObstacleCombatCallbackRuntime& runtime) {
    runtime.WriteWord(object, 0x84932cu);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 4u, 0u);
    return 0u;
}

std::uint32_t GobObstacleCombatCallback::Process(
    const std::uint32_t callback, const std::uint32_t context,
    const std::uint32_t argument,
    IGobObstacleCombatCallbackRuntime& runtime) {
    if (argument != 0u) return 0u;
    const auto object = runtime.ReadWord(callback + 8u);
    if (object != 0u) {
        runtime.InvokeObjectSlot80(object, {0u, 0u, 0u});
        runtime.ApplyEffectTypeToActor(object, context);
        runtime.WriteWord(
            runtime.ReadWord(runtime.MovementAdapter()) + 88u, 1u);
    }
    return 1u;
}

}  // namespace pal4
