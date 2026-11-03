#include "pal4/actor_animation_control.h"

namespace pal4 {

std::uint32_t ActorAnimationControl::SetAlpha(
    const std::uint32_t actor, const float alpha,
    const std::uint32_t queue, IActorAnimationControlRuntime& runtime) {
    const auto high_byte = static_cast<std::uint8_t>(
        runtime.FloatToInteger(alpha * 255.0f));
    const auto color = 0x00ffffffu |
                       (static_cast<std::uint32_t>(high_byte) << 24u);
    runtime.InvokeActorSlot24(actor, color);
    (void)runtime.ProcessClumpAtomics(runtime.ReadWord(actor + 20u), queue);
    if (runtime.ReadWord(actor + 644u)) {
        runtime.InvokeActorSlot24(actor + 624u, color);
        (void)runtime.ProcessClumpAtomics(runtime.ReadWord(actor + 644u),
                                          queue);
    }
    if (!runtime.ReadWord(actor + 1108u)) return 0;
    runtime.InvokeActorSlot24(actor + 1088u, color);
    return runtime.ProcessClumpAtomics(runtime.ReadWord(actor + 1108u),
                                       queue);
}

} // namespace pal4
