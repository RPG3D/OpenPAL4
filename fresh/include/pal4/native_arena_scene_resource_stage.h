#pragma once

#include "pal4/game_arena_scene_record.h"
#include "pal4/native_arena_collision_tree.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_scene_movement_asset.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>

struct RwCamera;

namespace pal4 {

class NativeWorldAsset;
class NativeWorldSectorRenderer;
class NativeMatFxScriptSession;
class NativeGameArenaManagerOwner;
class NativePalGameIVTextureStorage;
class NativePkFileManagerRuntime;
class NativePackageArchive;
class IGameArenaReleaseRuntime;

// Loads the world/BSP portion of PAL4's scene resource stage from original data.
class NativeArenaSceneResourceStage final {
public:
    NativeArenaSceneResourceStage(
        std::filesystem::path game_root,
        std::span<const GameArenaSceneRecord> scenes,
        NativeWorldAsset& world, std::uint32_t arena_address,
        NativePalGameIVTextureStorage& game, NativeGameArenaManagerOwner& arena_owner,
        NativePkFileManagerRuntime& packages);
    // All borrowed owners must outlive this stage, including after Release():
    // the archive cleanup precedes the world gate in IDA 0x51b270.
    ~NativeArenaSceneResourceStage();
    NativeArenaSceneResourceStage(const NativeArenaSceneResourceStage&) = delete;
    NativeArenaSceneResourceStage& operator=(
        const NativeArenaSceneResourceStage&) = delete;

    [[nodiscard]] bool Load(std::string_view arena, std::string_view area);
    // IDA 0x518a40 releases the previous Arena before loading a new scene.
    // Call after actor/GOB/EVF owners have detached their borrowed Clumps.
    [[nodiscard]] bool Switch(std::string_view arena, std::string_view area);
    [[nodiscard]] bool AttachCamera(RwCamera* camera);
    void DetachCamera() noexcept;
    // Original Scene lightmap pipeline, including live arena color parameters.
    void RenderWorld();
    [[nodiscard]] bool BuildCollisionTree();
    // Binds the matching original Scene table image at GameWorld +264.
    [[nodiscard]] std::uint32_t BindSceneRecord();
    // IDA51B3DC..51B440: call before destroying any queued scene borrowers.
    // Detaches optional world Clumps, restores callbacks and retains queue capacity.
    void ReleaseRenderBindings();
    // IDA51B448..51B4A7, before actor/player/object leave callbacks.
    void ReleaseSceneEnvironment();
    // IDA51B472: embedded dual sky clumps, before world resource release.
    void ReleaseSkyClumps();
    void Release();
    // Adapters borrow this stage and must not outlive it or overlap loading.
    std::unique_ptr<IGameArenaReleaseRuntime> CreateRenderReleaseRuntime();
    std::unique_ptr<IGameArenaReleaseRuntime> CreateEnvironmentReleaseRuntime();
    std::unique_ptr<IGameArenaReleaseRuntime> CreateWorldReleaseRuntime();
    // Call only after the shared world/pipeline tail completed successfully.
    void CompleteRelease();
    // Final lifecycle boundary, after all actor/collision borrowers retire.
    void ClearCollisionCache() noexcept { collision_tree_.Release(); }
    [[nodiscard]] std::size_t WorldTextAllocations() const noexcept {
        return text_memory_.OutstandingAllocations();
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }
    [[nodiscard]] bool LightmapConfigLoaded() const noexcept {
        return lightmap_config_loaded_;
    }
    [[nodiscard]] const NativeSceneMovementAsset& Movement() const noexcept {
        return movement_;
    }
    [[nodiscard]] NativeSceneMovementAsset& Movement() noexcept {
        return movement_;
    }
    [[nodiscard]] const NativeArenaCollisionTree& CollisionTree() const noexcept {
        return collision_tree_;
    }
    [[nodiscard]] NativeArenaCollisionTree& CollisionTree() noexcept {
        return collision_tree_;
    }

private:
    [[nodiscard]] bool LoadMountedScene(std::string_view world,
                                        std::string_view section,
                                        NativePackageArchive& archive);
    std::filesystem::path game_root_;
    std::span<const GameArenaSceneRecord> scenes_;
    NativeWorldAsset& world_;
    NativePalGameIVTextureStorage& game_;
    NativeGameArenaManagerOwner& arena_owner_;
    NativePkFileManagerRuntime& packages_;
    NativeBaseObjectMemory text_memory_;
    NativeSceneMovementAsset movement_;
    NativeArenaCollisionTree collision_tree_;
    std::unique_ptr<NativeMatFxScriptSession> shader_;
    std::unique_ptr<NativeWorldSectorRenderer> renderer_;
    RwCamera* camera_ = nullptr;
    std::uint32_t arena_address_;
    std::string world_path_;
    std::string world_name_;
    std::string area_name_;
    bool bound_ = false;
    std::string last_error_;
    bool lightmap_config_loaded_ = false;
};

}  // namespace pal4
