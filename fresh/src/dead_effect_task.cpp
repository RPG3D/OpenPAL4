#include "pal4/dead_effect_task.h"

namespace pal4 {

std::uint8_t DeadEffectTask::Process(
    const std::uint32_t task, IDeadEffectTaskRuntime& runtime) {
    const auto camera = runtime.CombatCamera(runtime.CombatSystem());
    auto effect = runtime.CreateEffect(
        camera, kEffectName, kEffectPosition, 1u, 0u);
    try {
        const auto actor = runtime.ReadWord(task + 60u);
        runtime.UpdatePosition(effect, runtime.ChildPosition(actor), 0u);
        std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
        const auto facet = runtime.ActorFacet(runtime.ReadWord(task + 60u));
        if (facet && facet + 608u != 0u)
            scale.fill(runtime.ReadFloat(facet + 620u) * 0.025f);
        runtime.UpdateScale(effect, scale, 0u);
        runtime.FinalizeEffect(effect);
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);
    return 1u;
}

}  // namespace pal4
