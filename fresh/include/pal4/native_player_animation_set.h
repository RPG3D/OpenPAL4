#pragma once

#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/player_role_animation_lifecycle.h"

#include <array>
#include <memory>

namespace pal4 {
struct PlayerManagerObjectState;
class NativePlayerActorAssets;

// Stable per-slot bindings; Role loading initializes every loaded actor.
class NativePlayerAnimationSet final : public IPlayerRoleAnimationLifecycle {
public:
    NativePlayerAnimationSet(PlayerManagerObjectState& players,
        NativePlayerActorAssets& actors, NativeBaseObjectMemory& memory,
        IGameObjectEmbeddedAnimationRuntime& lifecycle,
        std::filesystem::path game_root);
    ~NativePlayerAnimationSet();
    [[nodiscard]] NativeActorAnimationStateBinding& At(std::size_t index);
    [[nodiscard]] NativeActorAnimationStateBinding& For(BaseObjectState& object);
    void BindLoadedActor(std::uint32_t slot) override;
    void ReleaseActor(std::uint32_t slot) override;
    void Release();
    void UnbindSceneEvents() noexcept;
private:
    PlayerManagerObjectState& players_;
    NativePlayerActorAssets& actors_;
    NativeBaseObjectMemory& memory_;
    IGameObjectEmbeddedAnimationRuntime& lifecycle_;
    std::filesystem::path root_;
    std::array<std::unique_ptr<NativeActorAnimationStateBinding>, 4> bindings_;
};
}  // namespace pal4
