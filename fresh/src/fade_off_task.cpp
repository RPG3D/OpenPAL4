#include "pal4/fade_off_task.h"

namespace pal4 {

std::uint8_t FadeOffTask::Start(
    const std::uint32_t task, IFadeOffTaskRuntime& runtime) {
    auto duration = runtime.ReadFloat(task + 20u);
    if (duration < 0.1f) {
        duration = 0.1f;
        runtime.WriteFloat(task + 20u, duration);
    }
    runtime.WriteFloat(task + 64u, 255.0f);
    runtime.WriteFloat(task + 68u, 512.0f);
    runtime.WriteFloat(task + 72u, 1.0f);
    runtime.WriteFloat(task + 76u,
                       static_cast<float>(200.0 / static_cast<double>(duration)));
    runtime.SetActorState(runtime.ReadWord(task + 60u), 3u, 1u);
    return 1u;
}

std::uint32_t FadeOffTask::UpdateAlpha(
    const std::uint32_t task, const float step,
    IFadeOffTaskRuntime& runtime) {
    auto alpha = static_cast<float>(
        static_cast<double>(runtime.ReadFloat(task + 64u)) -
        static_cast<double>(step) *
            static_cast<double>(runtime.ReadFloat(task + 68u)));
    runtime.WriteFloat(task + 64u, alpha);
    if (alpha < 0.0f) {
        alpha = 0.0f;
        runtime.WriteWord(task + 64u, 0u);
    }
    const auto alpha_byte = static_cast<std::uint8_t>(
        runtime.FloatToLong(runtime.ReadFloat(task + 64u)));
    const auto color = 0x00ffffffu |
                       (static_cast<std::uint32_t>(alpha_byte) << 24u);
    const auto actor = runtime.ReadWord(task + 60u);
    auto result = actor;
    const auto facet = runtime.ActorFacet(actor);
    if (facet) {
        result = runtime.SetFacetColor(facet, color);
        runtime.WriteFloat(task + 72u,
            static_cast<float>(
                static_cast<double>(step) *
                    static_cast<double>(runtime.ReadFloat(task + 76u)) +
                static_cast<double>(runtime.ReadFloat(task + 72u))));
    }
    return (result & 0xffffff00u) | 1u;
}

}  // namespace pal4
