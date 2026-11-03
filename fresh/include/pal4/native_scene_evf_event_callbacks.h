#pragma once

#include "pal4/native_config_composite_collision_runtime.h"

namespace pal4 {

class NativeSceneEvfAssets;
class NativeSceneMovementAsset;
class NativeNewGameScriptSession;
class IPalGameObjectFactoryControlRuntime;
struct NativeSceneEvfRecord;

// Dispatches collision callbacks only while their EVF Clump is registered.
class NativeSceneEvfEventCallbacks final : public INativeCollisionEventCallbacks {
public:
    NativeSceneEvfEventCallbacks(
        NativeSceneEvfAssets& assets, NativeSceneMovementAsset& movement,
        NativeNewGameScriptSession& scripts,
        IPalGameObjectFactoryControlRuntime* controls = nullptr) noexcept;

    void Enter(std::uint32_t resource,
               std::uint32_t previous_state) override;
    void Leave(std::uint32_t resource) override;

private:
    [[nodiscard]] const NativeSceneEvfRecord& RequireResource(
        std::uint32_t resource) const;

    NativeSceneEvfAssets& assets_;
    NativeSceneMovementAsset& movement_;
    NativeNewGameScriptSession& scripts_;
    IPalGameObjectFactoryControlRuntime* controls_;
};

}  // namespace pal4
