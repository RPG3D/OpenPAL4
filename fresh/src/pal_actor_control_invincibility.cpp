#include "pal4/pal_actor_control_invincibility.h"

namespace pal4 {

bool IActorControlInvincibilityRuntime::NeedsUpdate(std::uint32_t*) {
    return false;
}
void IActorControlInvincibilityRuntime::Update(std::uint32_t*) {}
ActorControlEffectCreation IActorControlInvincibilityRuntime::CreateEffect(
    std::string_view, std::uint32_t, std::uint32_t, std::uint32_t) {
    return {};
}
void IActorControlInvincibilityRuntime::ResetTemporary(
    ActorControlEffectHandle& temporary) {
    temporary = {0xffffffffu, 0u};
}

std::uint32_t PalActorControlInvincibility::Enable(
    PalActorControlState& state, const std::uint32_t value,
    IActorControlInvincibilityRuntime& runtime) {
    const auto actor = state.words[1u];
    if (actor == 0u) {
        return 0u;
    }
    state.words[184u] = 1u;
    state.words[185u] = value;
    auto* handle = state.words.data() + 186u;
    if (runtime.NeedsUpdate(handle)) {
        runtime.Update(handle);
    }
    auto created = runtime.CreateEffect(
        "invincible", actor + 0xacu, 0xffffffffu, 0u);
    handle[0] = created.handle.object;
    handle[1] = created.handle.token;
    runtime.ResetTemporary(created.handle);
    return created.return_value;
}

}  // namespace pal4
