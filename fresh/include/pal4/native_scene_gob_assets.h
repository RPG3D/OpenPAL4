#pragma once

#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/native_gob_machine_runtime.h"
#include "pal4/native_gob_sound_runtime.h"
#include "pal4/native_gob_local_float_runtime.h"
#include "pal4/native_gob_obstacle_runtime.h"
#include "pal4/native_collision_event_router.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_arena_cache_save_section.h"
#include "pal4/native_pal_game_object_manager_owner.h"
#include "pal4/native_scene_gob_catalog.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pal4 {
class NativePalGameIVTextureStorage;
class NativeMilesSound2dRuntime;
class NativePkFileManagerRuntime;

// Owns the streamed models and records for one scene's GameObjs.gob.
class NativeSceneGobAssets final {
public:
    ~NativeSceneGobAssets();
    void BindSoundServices(std::function<NativeGobSoundServices()> factory) { sound_services_=std::move(factory); }
    void BindMachineSound(NativeMilesSound2dRuntime* sound) noexcept {machine_sound_=sound;}
    // Used during staged record loading, before initial visibility can trigger
    // repeat tasks. Cross-object/frame callbacks resolve the published scene.
    void BindMachineServices(std::function<NativeGobMachineServices()> factory) {machine_services_=std::move(factory);}
    void BindPlatformActorServices(std::function<NativeGobLocalFloatActorServices()> factory) { platform_actor_services_ = std::move(factory); }
    void BindObstacleActorServices(std::function<NativeGobObstacleActorServices()> factory) { obstacle_actor_services_ = std::move(factory); }
    NativeCollisionEventRouter& CollisionEvents(INativeCollisionEventCallbacks* fallback) { collision_events_.BindFallback(fallback); return collision_events_; }
    std::size_t ObstacleRuntimeCount() const noexcept { return obstacles_.size(); }
    std::uint32_t TouchMachine(std::uint32_t address, std::uint32_t event=0u);
    // Effects is a creating service borrowed through scene cleanup. The load
    // completes original record visibility dispatch after config and transforms.
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view region, std::string_view scene,
                            IGameResourceManagerRuntime& transforms,
                            NativeBaseObjectMemory& memory,
                            IRwDictSchemaRuntime& schema,
                            NativePkFileManagerRuntime& packages,
                            INativeGobEffectService& effects);
    void Clear();
    std::size_t RenderFrame(std::uint32_t camera);
    void CleanupScene(NativePalGameIVTextureStorage& game);
    [[nodiscard]] std::size_t RetainedManagerAllocations() const noexcept { return manager_allocations_; }

    [[nodiscard]] std::size_t Size() const noexcept { return models_.size(); }
    [[nodiscard]] bool SceneMemberMissing() const noexcept { return catalog_.SceneMemberMissing(); }
    [[nodiscard]] std::size_t AnimationBindingCount() const noexcept {
        return animations_.size();
    }
    [[nodiscard]] std::uint32_t NameTreeAddress() const noexcept {
        return manager_ ? manager_->Address() : 0u;
    }
    [[nodiscard]] std::uint32_t ManagerAddress() const noexcept {
        return manager_ ? manager_->Address() : 0u;
    }
    [[nodiscard]] std::size_t FactoryCount() const noexcept {
        return manager_ ? manager_->FactoryCount() : 0u;
    }
    [[nodiscard]] std::uint32_t FactoryForType(std::int32_t type) const {
        return manager_ ? manager_->FactoryForType(type) : 0u;
    }
    [[nodiscard]] const NativeSceneGobRecord* FindRecord(
        std::string_view name) const noexcept;
    [[nodiscard]] const NativeGobClumpAsset* FindModel(
        std::string_view name) const noexcept;
    [[nodiscard]] const NativeGobObjectImage* FindObject(
        std::string_view name) const;
    [[nodiscard]] NativeGobObjectImage* FindMutableObject(
        std::string_view name);
    [[nodiscard]] NativeGobObjectImage* FindMutableObjectByAddress(
        std::uint32_t address) noexcept;
    [[nodiscard]] std::optional<std::uint8_t> SetObjectVisible(
        std::string_view name, std::uint32_t visible);
    [[nodiscard]] std::vector<std::uint32_t> ObjectAddresses() const;
    void VisitObjects(const std::function<bool(std::uint32_t)>& visitor);
    [[nodiscard]] std::vector<NativeArenaCacheChild> CaptureCacheChildren();
    void ActivateAll();
    void DeactivateAll();
    [[nodiscard]] std::size_t AdvanceAnimations(float elapsed);
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    std::unique_ptr<NativeGobMachineRuntime> MakeMachineRuntime(
        NativeGobObjectImage&, const NativeSceneGobRecord&, NativeBaseObjectMemory&);
    NativeSceneGobCatalog catalog_;
    std::vector<std::unique_ptr<NativeGobClumpAsset>> models_;
    std::vector<std::unique_ptr<NativeGobObjectImage>> objects_;
    std::vector<std::unique_ptr<NativeGobAnimationBinding>> animations_;
    std::unique_ptr<NativePalGameObjectManagerOwner> manager_;
    NativeBaseObjectMemory* memory_ = nullptr;
    std::size_t manager_allocations_ = 0u;
    std::string last_error_;
    bool rendering_{};
    NativeMilesSound2dRuntime* machine_sound_{};
    std::function<NativeGobMachineServices()> machine_services_;
    std::unordered_map<std::uint32_t,std::unique_ptr<NativeGobMachineRuntime>> machines_;
    std::function<NativeGobLocalFloatActorServices()> platform_actor_services_;
    std::unordered_map<std::uint32_t,std::unique_ptr<NativeGobLocalFloatRuntime>> platforms_;
    std::function<NativeGobSoundServices()> sound_services_;
    std::unordered_map<std::uint32_t,std::unique_ptr<NativeGobSoundRuntime>> sounds_;
    NativeCollisionEventRouter collision_events_;
    std::function<NativeGobObstacleActorServices()> obstacle_actor_services_;
    std::unordered_map<std::uint32_t,std::unique_ptr<NativeGobObstacleRuntime>> obstacles_;
};

}  // namespace pal4
