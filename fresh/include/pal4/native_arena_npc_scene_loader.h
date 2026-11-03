#pragma once

#include "pal4/native_npc_info_reader.h"
#include "pal4/native_base_object_scene_frame.h"
#include "pal4/native_npc_manager_owner.h"
#include "pal4/native_arena_npc_scene_entry.h"
#include "pal4/native_npc_position_services.h"
#include "pal4/script_trade_npc_behavior.h"
#include "pal4/native_actor_behavior_orientation.h"

#include <filesystem>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeAnimatedActorAsset;
class NativeActorAnimationStateBinding;
class NativeBaseObjectMemory;
class NativeNpcObjectOwner;
class IRwDictSchemaRuntime;
class IGameObjectEmbeddedAnimationRuntime;
class IGameResourceManagerRuntime;
class NativeSceneMovementAsset;
class NativeArenaCollisionTree;
class NativePlayerTransformRuntime;
class IBaseObjectMaterialRuntime;
class NativeEventDocumentRuntime;
class NativeWorldPathPlannerOwner;
class NativeNpcBehaviorSession;

using NativeNpcSceneFrameResult=NativeBaseObjectSceneFrameResult;

// Retains the NPC manager across scenes; owns current records, objects and actors.
class NativeArenaNpcSceneLoader final : public INativeNpcPositionServices,
                                       public IScriptNpcBehaviorRuntime {
public:
    NativeArenaNpcSceneLoader(
        std::filesystem::path game_root, NativeBaseObjectMemory& memory,
        IRwDictSchemaRuntime& schema,
        IGameObjectEmbeddedAnimationRuntime& animation,
        IGameResourceManagerRuntime& transforms);
    ~NativeArenaNpcSceneLoader();
    NativeArenaNpcSceneLoader(const NativeArenaNpcSceneLoader&) = delete;
    NativeArenaNpcSceneLoader& operator=(const NativeArenaNpcSceneLoader&) = delete;

    // Requires an empty owner (new or CleanupScene); Clear is final teardown.
    void Load(std::string_view world, std::string_view scene);
    void LeaveAll();
    void BindWeaponRuntime(IBaseObjectNamedWeaponRuntime* runtime) {
        scene_entry_.BindWeaponRuntime(runtime);
    }
    void BindAttachedEffectRuntime(IBaseObjectAttachedEffectRuntime* runtime) {
        attached_ = runtime;
        scene_entry_.BindAttachedEffectRuntime(runtime);
    }
    void BindBehaviorRuntime(NativeWorldPathPlannerOwner&,NativeSceneMovementAsset&,
        NativePlayerTransformRuntime&,NativeActorBehaviorOrientationCaches&,
        std::uint32_t& identifier,std::function<std::uint32_t()> random);
    void StopBehaviorRuntime();
    NativeNpcBehaviorSession* BehaviorRuntime() noexcept { return behavior_.get(); }
    void Clear();
    // Retain the empty manager for the original post-release Scene entry.
    void CleanupScene(std::uint32_t game, IBaseObjectMaterialRuntime& materials);
    [[nodiscard]] std::uint32_t EnterAll(
        std::uint32_t arena, NativeSceneMovementAsset& movement,
        NativeArenaCollisionTree& collision);
    [[nodiscard]] NativeNpcSceneFrameResult RenderFrame(
        std::uint32_t camera, NativePlayerTransformRuntime& transforms);
    [[nodiscard]] std::uint32_t ProcessCameraPass(
        std::uint32_t camera, NativePlayerTransformRuntime& transforms);
    [[nodiscard]] std::uint32_t UpdateWorldFrame(
        float elapsed, std::uint32_t mode, NativeArenaCollisionTree& collision,
        NativePlayerTransformRuntime& transforms);
    [[nodiscard]] bool SetObjectStateFlag(std::string_view name,
                                          std::uint32_t value);
    [[nodiscard]] bool SetObjectStateFlag(std::uint32_t npc,
                                          std::uint32_t value);
    [[nodiscard]] bool OwnsObject(std::uint32_t npc) const noexcept override;
    [[nodiscard]] bool SelectAction(std::uint32_t npc,
                                    std::string_view action);
    std::uint32_t NpcManager() override;
    std::uint32_t FindNpc(std::uint32_t manager,std::string_view name) override;
    std::uint32_t FindNpcInfo(std::uint32_t manager,std::string_view name) override;
    void CleanupEventsAndScripts(std::uint32_t npc,std::uint32_t flag) override;
    std::uint32_t SetActionCode(std::uint32_t component,std::string_view code) override;
    void ProcessEventsAndClear(std::uint32_t npc,std::uint32_t info,std::uint32_t flag) override;
    std::uint32_t NpcByName(std::uint32_t,std::string_view) override;
    void SetNpcPosition(std::uint32_t,const PlayerPosition&,std::uint32_t) override;
    std::int32_t SetNpcAction(std::uint32_t,std::string_view) override;
    [[nodiscard]] bool AdvanceAction(std::uint32_t npc, float elapsed);
    [[nodiscard]] NativeActorAnimationStateBinding& AnimationBinding(std::uint32_t npc);
    [[nodiscard]] NativeEventDocumentRuntime& BehaviorDocuments(std::uint32_t npc);
    [[nodiscard]] std::uint32_t LeaveScriptNpc(std::uint32_t npc);
    [[nodiscard]] std::uint32_t EnterScriptNpc(std::uint32_t npc);
    [[nodiscard]] std::size_t EnteredCount() const noexcept {
        return scene_entry_.Count();
    }
    [[nodiscard]] NativeNpcManagerOwner& Manager() noexcept { return manager_; }
    [[nodiscard]] const NativeNpcManagerOwner& Manager() const noexcept { return manager_; }
    [[nodiscard]] std::size_t Count() const noexcept { return objects_.size(); }

private:
    struct Entry;
    std::filesystem::path game_root_;
    NativeBaseObjectMemory& memory_;
    IRwDictSchemaRuntime& schema_;
    IGameObjectEmbeddedAnimationRuntime& animation_;
    IGameResourceManagerRuntime& transforms_;
    NativeNpcInfoReader reader_;
    NativeNpcManagerOwner manager_;
    NativeArenaNpcSceneEntry scene_entry_;
    IBaseObjectAttachedEffectRuntime* attached_{};
    std::vector<std::unique_ptr<Entry>> objects_;
    std::unique_ptr<NativeNpcBehaviorSession> behavior_;
};

}  // namespace pal4
