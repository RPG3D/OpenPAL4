#pragma once

#include "pal4/native_actor_animation_state_binding.h"

#include <array>
#include <memory>

namespace pal4 {
struct PlayerManagerObjectState;
class NativePlayerActorAssets;

// Stable per-slot bindings. Construct lazily so dormant players do not acquire
// animation records until scene or save restoration needs them.
class NativePlayerAnimationSet final {
public:
    NativePlayerAnimationSet(PlayerManagerObjectState& players,
        NativePlayerActorAssets& actors, NativeBaseObjectMemory& memory,
        IGameObjectEmbeddedAnimationRuntime& lifecycle,
        std::filesystem::path game_root);
    ~NativePlayerAnimationSet();
    [[nodiscard]] NativeActorAnimationStateBinding& At(std::size_t index);
    [[nodiscard]] NativeActorAnimationStateBinding& For(BaseObjectState& object);
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
