#pragma once
#include "pal4/native_combat_scene_actors.h"
#include <cstddef>
#include <memory>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatPlayerModelCache;
class NativeScriptEventRouter;
// Typed Player half of existing WorldManager scene events. Registered caches,
// manager and router outlive this adapter; world binding closes before caches.
class NativeCombatPlayerSceneActors final : public INativeCombatSceneActors {
public:
    NativeCombatPlayerSceneActors(NativeCombatPlayerManager&,NativeScriptEventRouter&);
    ~NativeCombatPlayerSceneActors();
    // Empty/offstage caches may join an already bound world before creation
    // notification. Reject live models, stage members, duplicate/active cursors.
    void Register(NativeCombatPlayerModelCache&);
    // Retire the borrowed cache before its actor owner is released. Requires
    // unbound scene events and an offstage, still-owned Player.
    void Unregister(NativeCombatPlayerModelCache&);
    void BindSceneEvents() override;
    void UnbindSceneEvents() noexcept override;
    std::uint32_t SceneChild(std::uint32_t actor) override;
    // The first roster may contain a fourth, unmodeled reserve. Only actual
    // stage members must have current models after the creation callbacks.
    std::size_t ValidateStageModels();
    void ActivateSceneModel(std::uint32_t actor,std::uint32_t world,std::uint32_t queue) override;
    void PositionSceneModel(std::uint32_t actor,const ResourceVec3&) override;
    void RestoreSceneCallbacks() override;
    void RenderModels(std::uint32_t camera,std::uint32_t slot);
    void UpdateStageModels(float elapsed);
    // Complete53B3E0 phases: stage child56, then every first-list actor728,
    // including offstage Players. Real activity services required if populated.
    void UpdateActors(float elapsed);
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
