#include "pal4/combat_console.h"

namespace pal4 {
namespace {

std::uint32_t Animate(const std::uint32_t console,
                      const std::int32_t index, const float progress,
                      const std::uint32_t slot_offset,
                      ICombatConsoleAnimationRuntime& runtime) {
    const auto image = runtime.ReadWord(
        console + slot_offset +
        4u * static_cast<std::uint32_t>(index));
    const auto left = runtime.ReadFloat(console + 44u);
    const auto right = runtime.ReadFloat(console + 52u);
    const auto x = (right - left) * progress * 0.015625f + left;
    return runtime.SetPosition(image, 1u, x,
                               runtime.ReadFloat(console + 48u));
}

}  // namespace

std::uint32_t CombatConsoleWindow::AnimatePosition(
    const std::uint32_t console, const std::int32_t index,
    const float progress, ICombatConsoleAnimationRuntime& runtime) {
    const auto state = runtime.GameState();
    if (state == 1u) return state;
    if (index >= 3) return static_cast<std::uint32_t>(index);
    return Animate(console, index, progress, 224u, runtime);
}

std::uint32_t CombatConsoleWindow::AnimatePositionSecond(
    const std::uint32_t console, const std::int32_t index,
    const float progress, ICombatConsoleAnimationRuntime& runtime) {
    const auto state = runtime.GameState();
    if (state == 0u) return state;
    if (index >= 6) return static_cast<std::uint32_t>(index);
    return Animate(console, index, progress, 236u, runtime);
}

}  // namespace pal4
