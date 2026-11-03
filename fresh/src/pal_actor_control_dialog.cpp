#include "pal4/game_vector_math.h"
#include "pal4/pal_actor_control_dialog.h"

#include <cmath>

namespace pal4 {

std::uint32_t IActorControlDialogRuntime::IterateAndProcess(
    std::uint32_t) { return 0u; }
std::uint32_t IActorControlDialogRuntime::ReadWord(
    std::uint32_t) { return 0u; }
float IActorControlDialogRuntime::ReadFloat(std::uint32_t) { return 0.0f; }
bool IActorControlDialogRuntime::DialogEnabled(std::uint32_t) {
    return false;
}
bool IActorControlDialogRuntime::RestrictHeight(std::uint32_t) {
    return false;
}
double IActorControlDialogRuntime::ActorProperty(std::uint32_t) {
    return 0.0;
}
float IActorControlDialogRuntime::PlanarMagnitude(
    ActorControlEffectPoint offset) {
    return static_cast<float>(GameVectorMath::ApproximateMagnitude(
        {offset.x,0.0f,offset.z}));
}
void IActorControlDialogRuntime::AddActiveItem(
    std::uint32_t, float) {}

std::uint32_t PalActorControlDialog::Release(
    PalActorControlState& state, IActorControlDialogRuntime& runtime) {
    const auto actor = state.words[1];
    if (actor == 0u) return 0u;
    const auto remaining = state.words[19] - 1u;
    if (remaining == 0u) return runtime.IterateAndProcess(actor);
    return remaining;
}

std::uint32_t PalActorControlDialog::Visit(
    std::uint32_t dialog, std::uint32_t actor,
    IActorControlDialogRuntime& runtime) {
    if (runtime.ReadWord(dialog + 440u) == 0u ||
        runtime.ReadWord(dialog + 388u) != 0u ||
        !runtime.DialogEnabled(dialog))
        return dialog;

    const ActorControlEffectPoint dialog_position{
        runtime.ReadFloat(dialog + 172u),
        runtime.ReadFloat(dialog + 176u),
        runtime.ReadFloat(dialog + 180u)};
    const ActorControlEffectPoint actor_position{
        runtime.ReadFloat(actor + 172u),
        runtime.ReadFloat(actor + 176u),
        runtime.ReadFloat(actor + 180u)};
    if (runtime.RestrictHeight(dialog) &&
        runtime.ActorProperty(actor) <= std::fabs(
            static_cast<double>(actor_position.y - dialog_position.y)))
        return dialog;

    const ActorControlEffectPoint planar{
        actor_position.x - dialog_position.x, 0.0f,
        actor_position.z - dialog_position.z};
    const auto distance = runtime.PlanarMagnitude(planar);
    if (distance < runtime.ReadFloat(actor + 236u) * 3.0f)
        runtime.AddActiveItem(dialog, 1.5f);
    return dialog;
}

}  // namespace pal4
