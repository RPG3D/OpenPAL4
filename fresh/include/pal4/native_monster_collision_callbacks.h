#pragma once

#include "pal4/native_config_composite_collision_runtime.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;
class NativePalGameIVTextureStorage;
class NativePalGameIVStateList;
class NativeGameArenaManagerOwner;
class NativeSceneMovementAsset;
class NativeEventDocumentRuntime;
class NativeScriptCombatConfig;
class NativeScriptCombatStart;
class IMonsterInteractionRuntime;
class IMonsterEncounterRuntime;

struct NativeMonsterCollisionServices {
    NativeBaseObjectMemory& memory;
    NativeMonsterManagerOwner& monsters;
    NativePalGameIVTextureStorage& game;
    NativePalGameIVStateList& states;
    NativeGameArenaManagerOwner& arena;
    NativeSceneMovementAsset& movement;
    NativeEventDocumentRuntime& documents;
    NativeScriptCombatConfig& config;
    NativeScriptCombatStart& request;
    std::uint32_t actor_control;
    // Prepare dependencies without ticking the simulation or advancing RNG.
    std::function<void()> prepare;
    std::function<IMonsterInteractionRuntime&()> interaction;
    std::function<IMonsterEncounterRuntime&()> encounter;
    std::function<std::string(std::string_view)> floor_path;
};

// Routes registered monster+6D4 callbacks to5A9530/+8 ReturnOne. Other
// resources retain their own validated dispatcher (currently the EVF owner).
// Dependencies and fallback outlive this scene-owned adapter.
class NativeMonsterCollisionCallbacks final : public INativeCollisionEventCallbacks {
public:
    NativeMonsterCollisionCallbacks(NativeMonsterCollisionServices,
        INativeCollisionEventCallbacks& fallback);
    ~NativeMonsterCollisionCallbacks();
    void Enter(std::uint32_t resource, std::uint32_t previous_state) override;
    void Leave(std::uint32_t resource) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
