#pragma once

#include "pal4/pal_actor_control_invincibility.h"

namespace pal4 {
class NativeEffectSceneSession;

// Borrows ActorControl's real status/handle words and the creating arena effect
// group. Scope exit does not cancel the effect: ActorControl owns the handle and
// the existing424750 frame owns countdown, position updates and expiry.
class NativeActorControlInvincibility final : private IActorControlInvincibilityRuntime {
public:
    NativeActorControlInvincibility(PalActorControlState&, NativeEffectSceneSession&);
    void Enable(float seconds);
    [[nodiscard]] bool Owns(const PalActorControlState&) const noexcept;
    bool NeedsUpdate(std::uint32_t* handle) override;
    void Update(std::uint32_t* handle) override;
    void UpdatePosition(std::uint32_t* handle, std::uint32_t actor, bool& additive);
private:
    void RequireHandle(const std::uint32_t*) const;
    ActorControlEffectCreation CreateEffect(std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t) override;
    void ResetTemporary(ActorControlEffectHandle&) override;
    PalActorControlState& state_;
    NativeEffectSceneSession& effects_;
};
}  // namespace pal4
