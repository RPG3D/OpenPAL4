#pragma once

#include "pal4/collection_camera_bootstrap.h"

#include <functional>

namespace pal4 {
class NativeCameraManagerOwner;

// Borrowed adapter for uiSceneManager's 444F40/445000. WorldCamera receives
// the actual SDK camera; the world owner supplies its existing membership path.
class NativeCollectionCamera final : private ICollectionCameraRuntime {
public:
    using WorldCamera = std::function<void(CollectionState&, std::uint32_t)>;
    using Log = std::function<std::uint32_t(std::string_view, std::string_view, std::uint32_t)>;
    NativeCollectionCamera(NativeCameraManagerOwner& cameras, WorldCamera bind, Log log);
    [[nodiscard]] std::uint32_t Initialize(CollectionState& state);
    // Original 445000 returns UiCamera's CameraData, without activating it.
    [[nodiscard]] std::uint32_t UiCameraData();

private:
    std::uint32_t InitializeCameraSubsystem() override;
    std::uint32_t FindCamera(std::uint32_t manager, std::string_view name) override;
    std::uint32_t InternalObject(std::uint32_t camera) override;
    void SetAngles(std::uint32_t internal, std::uint32_t first, std::uint32_t second,
                   std::uint32_t third, std::uint32_t mode) override;
    void SetLookAt(std::uint32_t internal, const std::array<std::uint32_t, 3>& words,
                   std::uint32_t mode) override;
    void SetScalar(std::uint32_t internal, std::uint32_t bits, std::uint32_t mode) override;
    std::uint32_t CameraContainer(std::uint32_t camera) override;
    void SetWorldCamera(CollectionState& state, std::uint32_t camera) override;
    std::uint32_t InitializeLogger() override;
    std::uint32_t AppendLog(std::uint32_t logger, std::string_view format,
                            std::string_view source, std::uint32_t line) override;
    NativeCameraManagerOwner& cameras_;
    WorldCamera bind_;
    Log log_;
};
}  // namespace pal4
