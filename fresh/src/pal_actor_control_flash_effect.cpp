#include "pal4/pal_actor_control_flash_effect.h"

namespace pal4 {

bool IActorControlFlashRuntime::NeedsUpdate(std::uint32_t*) { return false; }
void IActorControlFlashRuntime::HandleEvent(std::uint32_t*, std::uint32_t) {}
ActorControlEffectHandle IActorControlFlashRuntime::CreateEffect(
    std::string_view, std::uint32_t, std::uint32_t, std::uint32_t) {
    return {};
}
void IActorControlFlashRuntime::ResetTemporary(
    ActorControlEffectHandle& temporary) {
    temporary = {0xffffffffu, 0u};
}
std::uint32_t IActorControlFlashRuntime::ReadEffectProperty(
    std::uint32_t) { return 0u; }
void IActorControlFlashRuntime::UpdateProperties(
    std::uint32_t*, std::uint32_t) {}

bool PalActorControlFlashEffect::Update(
    PalActorControlState& state, const std::uint32_t context_address,
    IActorControlFlashRuntime& runtime) {
    if (context_address == 0u) {
        return false;
    }
    auto* handle = state.words.data() + 279u;
    if (runtime.NeedsUpdate(handle)) {
        runtime.HandleEvent(handle, 0u);
    }
    if (state.words[33u] == 2u) {
        auto temporary = runtime.CreateEffect(
            "flash", kParameterAddress, 1u, 0u);
        handle[0] = temporary.object;
        handle[1] = temporary.token;
        runtime.ResetTemporary(temporary);
        if (runtime.NeedsUpdate(handle)) {
            runtime.UpdateProperties(
                handle, runtime.ReadEffectProperty(context_address));
        }
    }
    return true;
}

}  // namespace pal4
