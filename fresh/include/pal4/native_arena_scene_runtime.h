#pragma once

#include "pal4/game_arena_scene_runtime_loader.h"

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace pal4 {

class IPlayerManagerOperationsRuntime;
class IBaseObjectMaterialRuntime;
class NativeArenaSceneResourceStage;
class NativeGameArenaManagerOwner;
class NativePkFileManagerRuntime;
class NativePlayerManagerSession;
class NativeSceneRecordCatalog;
class NativeArenaNpcSceneLoader;
class NativeSceneGobAssets;
class NativeArenaGobSceneSession;
class NativeSceneEvfAssets;
class NativeCameraManagerOwner;
class NativeActorControlSession;

// Binds the beginning of IDA 0x519310 to owned scene, PK and player services.
class NativeArenaSceneRuntime final : private IGameArenaSceneRuntime {
public:
    NativeArenaSceneRuntime(
        NativeGameArenaManagerOwner& arena,
        NativeArenaSceneResourceStage& resources,
        NativePkFileManagerRuntime& packages,
        NativePlayerManagerSession& players,
        IPlayerManagerOperationsRuntime& player_reset,
        const NativeSceneRecordCatalog& scenes,
        NativeArenaNpcSceneLoader& npcs,
        NativeSceneGobAssets& objects,
        NativeArenaGobSceneSession& object_entries,
        NativeSceneEvfAssets& effects,
        NativeCameraManagerOwner& cameras,
        NativeActorControlSession& actor_control,
        IBaseObjectMaterialRuntime& player_materials,
        std::function<void(std::string_view, std::string_view)> load_objects,
        std::function<void(std::string_view, std::string_view)> load_monsters,
        std::function<void(std::string_view, std::string_view)> load_effects,
        std::function<void(std::string_view, std::string_view)> load_pathway,
        std::function<bool(float)> update_progress,
        std::function<void()> dispatch_ui_event = {},
        std::function<void(std::string_view, std::string_view)> update_cache = {},
        std::function<void()> refresh_object_sprites = {},
        std::function<void()> for_each_monster = {},
        std::function<bool(std::string_view, std::string_view)>
            execute_scene_script = {},
        std::function<void(std::string_view, std::string_view)>
            set_background_music = {});

    [[nodiscard]] bool Load(std::string_view scene_name);

private:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::string_view ReadText(std::uint32_t address) override;
    void SetSceneName(std::uint32_t address,
                      std::string_view text) override;
    [[nodiscard]] std::uint32_t NavClump() override;
    void BuildCollisionTree(std::uint32_t world,
                            std::uint32_t clump) override;
    void ShowMissingNavmesh(std::string_view message,
                            std::string_view title) override;
    [[nodiscard]] std::string_view EffectRoot() override;
    void LoadEffectRecords(std::string_view root,
                           std::string_view world,
                           std::string_view scene) override;
    [[nodiscard]] std::span<const GameArenaSceneRecord>
        SceneRecords() override;
    void UpdateProgress(float value) override;
    [[nodiscard]] std::uint32_t Call(
        GameArenaRuntimeOperation operation,
        std::uint32_t first, std::uint32_t second,
        std::string_view first_text,
        std::string_view second_text) override;

    NativeGameArenaManagerOwner& arena_;
    NativeArenaSceneResourceStage& resources_;
    NativePkFileManagerRuntime& packages_;
    NativePlayerManagerSession& players_;
    IBaseObjectMaterialRuntime& player_materials_;
    IPlayerManagerOperationsRuntime& player_reset_;
    const NativeSceneRecordCatalog& scenes_;
    NativeArenaNpcSceneLoader& npcs_;
    NativeSceneGobAssets& objects_;
    NativeArenaGobSceneSession& object_entries_;
    NativeSceneEvfAssets& effects_;
    NativeCameraManagerOwner& cameras_;
    NativeActorControlSession& actor_control_;
    std::function<void(std::string_view, std::string_view)> load_objects_;
    std::function<void(std::string_view, std::string_view)> load_monsters_;
    std::function<void(std::string_view, std::string_view)> load_effects_;
    std::function<void(std::string_view, std::string_view)> load_pathway_;
    std::function<bool(float)> update_progress_;
    std::function<void()> dispatch_ui_event_;
    std::function<void(std::string_view, std::string_view)> update_cache_;
    std::function<void()> refresh_object_sprites_;
    std::function<void()> for_each_monster_;
    std::function<bool(std::string_view, std::string_view)>
        execute_scene_script_;
    std::function<void(std::string_view, std::string_view)>
        set_background_music_;
};

}  // namespace pal4
