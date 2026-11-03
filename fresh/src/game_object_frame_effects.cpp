#include "pal4/game_object_frame_effects.h"

namespace pal4 {

std::int32_t GameObjectFrameEffects::RenderTransform(
    const float elapsed, IGameObjectFrameEffectRuntime& runtime) {
    return runtime.RenderTransform(static_cast<double>(elapsed));
}

std::uint8_t GameObjectFrameEffects::Draw(
    const std::uint32_t effect, const std::uint32_t register_b,
    const std::uint32_t register_si,
    const std::uint32_t first_argument_address,
    const std::uint32_t second_argument,
    const std::uint32_t third_argument,
    const std::uint32_t fourth_argument,
    IGameObjectFrameEffectRuntime& runtime) {
    return static_cast<std::uint8_t>(runtime.DrawEffect(
        effect, register_b, register_si,
        runtime.ReadWord(first_argument_address), second_argument,
        third_argument, fourth_argument));
}

}  // namespace pal4
