#include "pal4/combat_transition_overlay.h"

#include <array>

namespace pal4 {

std::uint32_t CombatTransitionOverlay::Construct(
    std::uint32_t object, ICombatTransitionRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    for (const auto offset : std::array{4u, 8u, 12u, 16u})
        runtime.WriteWord(object + offset, 0u);
    return object;
}

std::uint32_t CombatTransitionOverlay::Destroy(
    std::uint32_t object, std::uint32_t flags,
    ICombatTransitionRuntime& runtime) {
    ResetState(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

void CombatTransitionOverlay::ResetState(
    std::uint32_t object, ICombatTransitionRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, kBaseVtable);
}

std::uint32_t CombatTransitionOverlay::Initialize(
    std::uint32_t object, ICombatTransitionRuntime& runtime) {
    runtime.InitializeEffectParameters(runtime.ScreenEffect());
    runtime.WriteWord(object + 12u, 1u);
    runtime.WriteFloat(object + 8u, 5.0f);
    runtime.WriteFloat(object + 4u, 5.0f);
    runtime.ProcessScript6();
    runtime.WriteWord(object + 16u,
                      runtime.LoadTexture(runtime.TextureDictionary(),
                                          kTextureName));
    runtime.CleanupScripts();
    return 1u;
}

std::uint32_t CombatTransitionOverlay::Draw(
    std::uint32_t object, std::uint32_t frame,
    ICombatTransitionRuntime& runtime) {
    if (runtime.ReadWord(object + 12u) == 0u) return 0u;
    const auto texture = runtime.ReadWord(object + 16u);
    if (texture == 0u) return 0u;
    const auto config = runtime.GameConfig();
    const auto width = runtime.ReadWord(config);
    const auto height = runtime.ReadWord(config + 4u);
    const auto alpha = runtime.Ftol(
        static_cast<double>(runtime.ReadFloat(object + 4u)) /
        static_cast<double>(runtime.ReadFloat(object + 8u)) * 255.0);
    return runtime.DrawRectangle(frame, 0u, 0u, width, height, texture,
                                 255u, 255u, 255u, alpha);
}

void CombatTransitionOverlay::AdvanceTimer(
    std::uint32_t object, float delta,
    ICombatTransitionRuntime& runtime) {
    if (runtime.ReadWord(object + 12u) == 0u) return;
    const auto remaining = static_cast<double>(runtime.ReadFloat(object + 4u)) -
                           static_cast<double>(delta);
    runtime.WriteFloat(object + 4u, static_cast<float>(remaining));
    if (remaining <= 0.0)
        runtime.InitializeStateAndUI(runtime.GameInstance(), 0u, 0u);
}

std::uint32_t CombatTransitionOverlay::ResetProperties(
    std::uint32_t object, ICombatTransitionRuntime& runtime) {
    runtime.WriteWord(object + 16u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    return 1u;
}

}  // namespace pal4
