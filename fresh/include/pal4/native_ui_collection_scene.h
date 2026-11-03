#pragma once

#include "pal4/native_collection_camera.h"
#include "pal4/native_collection_world_update.h"
#include "pal4/native_world_asset.h"

#include <filesystem>
#include <memory>
#include <string>

namespace pal4 {
class NativeUiFrameUpdate;
class NativeUiCollectionObjects;
class NativePkFileManagerRuntime;
class NativePalGameIVTextureStorage;

// Owns original UI world assets and optional component resources. Camera/frame,
// package/game owners must outlive this scene. UI effects have separate owners.
class NativeUiCollectionScene final {
public:
    NativeUiCollectionScene(NativeCameraManagerOwner& cameras,
                            NativeUiFrameUpdate& frames,
                            NativeCollectionCamera::Log log);
    ~NativeUiCollectionScene();
    NativeUiCollectionScene(const NativeUiCollectionScene&) = delete;
    NativeUiCollectionScene& operator=(const NativeUiCollectionScene&) = delete;

    void Load(const std::filesystem::path& root, std::string_view name);
    [[nodiscard]] std::uint32_t SetupCamera();
    [[nodiscard]] std::uint32_t UiCameraData();
    void Release();
    NativeUiCollectionObjects& EnableComponents(NativePkFileManagerRuntime&,
                                               NativePalGameIVTextureStorage&);
    [[nodiscard]] CollectionState& Collection() noexcept { return collection_; }
    [[nodiscard]] const NativeWorldAsset& Asset() const noexcept { return world_; }

private:
    void AttachCamera(CollectionState& collection, std::uint32_t camera);
    NativeUiFrameUpdate& frames_;
    CollectionState collection_{};
    NativeWorldAsset world_;
    std::unique_ptr<NativeUiCollectionObjects> objects_;
    NativeCollectionWorldUpdate updates_;
    NativeCollectionCamera camera_;
    std::uint32_t attached_camera_ = 0;
    bool bound_ = false;
    std::filesystem::path root_;
    std::string name_;
};
}  // namespace pal4
