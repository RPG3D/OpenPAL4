#include "pal4/effect_system_render_state.h"

namespace pal4 {

void EffectSystem::SetRenderStates(
    std::uint32_t system, IEffectSystemRenderStateRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u ||
        runtime.ReadWord(system + 172u) == 0u)
        return;

    const auto prior27 = runtime.GetRenderState(27u);
    const auto prior19 = runtime.GetRenderState(19u);
    const auto prior20 = runtime.GetRenderState(20u);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, 6u);

    const auto prior7 = runtime.GetRenderState(7u);
    const auto prior14 = runtime.GetRenderState(14u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 1u);
    runtime.SortAtomicGeometries(system + 52u);
    runtime.SetRenderState(7u, prior7);
    runtime.SetRenderState(14u, prior14);
    runtime.SetRenderState(27u, prior27);
    runtime.SetRenderState(19u, prior19);
    runtime.SetRenderState(20u, prior20);
}

}  // namespace pal4
