#include "pal4/actor_manager_scale_metrics.h"

namespace pal4 {

std::array<float, 3> ActorManagerScaleMetrics::Scale(
    std::uint32_t actor, IActorManagerScaleMetricsRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    if (child == 0u || runtime.ReadWord(actor + 804u) != 2u)
        return {1.0f, 1.0f, 1.0f};
    const auto scale = runtime.ReadFloat(child + 620u) * 0.025f;
    return {scale, scale, scale};
}

double ActorManagerScaleMetrics::ClampedSize(
    std::uint32_t actor, IActorManagerScaleMetricsRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    if (child == 0u) return 70.0;
    const double size = static_cast<double>(runtime.ReadFloat(child + 620u)) +
                        10.0;
    if (size > 190.0) return 190.0;
    if (size < 70.0) return 70.0;
    return size;
}

}  // namespace pal4
