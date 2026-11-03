#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_script_player_position_runtime.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeSceneGobAssets;

struct NativeMinimapSaveSnapshot {
    std::uint32_t header = 0u;
    std::vector<std::vector<std::uint32_t>> elements;
};

class NativeMinimapManagerOwner final : public INativeScriptGrid {
public:
    NativeMinimapManagerOwner();
    ~NativeMinimapManagerOwner();
    NativeMinimapManagerOwner(const NativeMinimapManagerOwner&) = delete;
    NativeMinimapManagerOwner& operator=(const NativeMinimapManagerOwner&) = delete;

    [[nodiscard]] bool Initialize(const std::filesystem::path& game_root);
    [[nodiscard]] bool LoadSceneWithoutLayers(
        const std::filesystem::path& game_root,
        std::string_view folder, std::string_view scene);
    [[nodiscard]] bool LoadScene(
        const std::filesystem::path& game_root,
        std::string_view folder, std::string_view scene);
    void BindSceneObjects(const NativeSceneGobAssets& objects) noexcept {
        scene_objects_ = &objects;
    }
    [[nodiscard]] bool AddLeadingRole();
    [[nodiscard]] bool ActivateLeadingRole();
    void RefreshObjectSprites(const NativeSceneGobAssets& objects);
    void SetupTexture(const std::array<std::uint32_t, 4>& region);
    void SetLevel(std::uint32_t level);
    void Draw(std::uint32_t camera_slot);
    void UpdateGrid(std::uint32_t position,
                    std::uint32_t context) override;
    void Destroy();

    [[nodiscard]] std::uint32_t Address() const noexcept { return manager_; }
    [[nodiscard]] std::uint32_t ActiveIcon() const;
    [[nodiscard]] NativeMinimapSaveSnapshot CaptureSaveSnapshot();
    void RestoreSaveSnapshot(const NativeMinimapSaveSnapshot& snapshot);
    [[nodiscard]] std::uint32_t GridUpdates() const noexcept {
        return grid_updates_;
    }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    class CollectionRuntime;
    class IconRuntime;
    class AddRuntime;
    class ActiveRuntime;
    class ManagerRuntime;
    class ClearRuntime;
    class ScriptRuntime;
    class LayerFileRuntime;
    class TextureRuntime;
    class SnapshotRuntime;
    class RestoreRuntime;

    NativeBaseObjectMemory memory_;
    NativePackageArchive archive_;
    std::unique_ptr<CollectionRuntime> collection_runtime_;
    std::unique_ptr<IconRuntime> icon_runtime_;
    std::unique_ptr<AddRuntime> add_runtime_;
    std::unique_ptr<ActiveRuntime> active_runtime_;
    std::unique_ptr<ManagerRuntime> manager_runtime_;
    std::unique_ptr<ClearRuntime> clear_runtime_;
    std::unique_ptr<ScriptRuntime> script_runtime_;
    std::unique_ptr<TextureRuntime> texture_runtime_;
    std::filesystem::path game_root_;
    const NativeSceneGobAssets* scene_objects_ = nullptr;
    std::uint32_t manager_ = 0u;
    std::uint32_t grid_updates_ = 0u;
};

}  // namespace pal4
