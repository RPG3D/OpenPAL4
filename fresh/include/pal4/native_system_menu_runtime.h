#pragma once

#include "pal4/system_menu_state.h"
#include <filesystem>
#include <functional>

namespace pal4 {
class NativeCameraManagerOwner;
class NativeUiCollectionScene;
class NativeUiCameraSnapshot;
class NativeUiFrameUpdate;
class NativeEffectSceneSession;

// Required application endpoints for registered windows and render scheduling.
// Resource/camera/frame/effect operations belong to the native owners below.
struct NativeSystemMenuServices {
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)> event;
    std::function<void(std::uint32_t, bool)> edges;
    std::function<void(std::uint32_t, std::uint32_t)> pre_render;
    std::function<std::uint32_t(std::uint32_t, std::uint32_t)> render;
};

// Borrows all dependencies and the console word. The caller binds the UI effect
// group before use and returns to Scene/unbinds state dispatch before destruction.
class NativeSystemMenuRuntime final : public ISystemMenuRuntime {
public:
    NativeSystemMenuRuntime(std::uint32_t manager, std::uint32_t& console,
        std::filesystem::path root, NativeCameraManagerOwner& cameras,
        NativeUiCollectionScene& scene, NativeUiCameraSnapshot& snapshot,
        NativeUiFrameUpdate& frames, NativeEffectSceneSession& effects,
        NativeSystemMenuServices services);
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override;
    std::uint32_t Console() override;
    void AddUIElement(std::uint32_t manager, std::string_view name) override;
    void SetupUICamera(std::uint32_t collection, std::uint32_t enabled) override;
    void CopyActiveCameraRaster(std::uint32_t manager) override;
    void GetActiveCamera(std::uint32_t collection) override;
    std::uint32_t CameraManager() override;
    std::uint32_t UICameraName() override;
    void ActivateCamera(std::uint32_t manager, std::uint32_t name) override;
    void DispatchEvent(std::uint32_t manager, std::uint32_t descriptor,
                       std::uint32_t first, std::uint32_t second) override;
    void ToggleWindows(std::uint32_t manager, bool enabled) override;
    void PreRenderUI(std::uint32_t manager, std::uint32_t frame) override;
    std::uint32_t RenderUI(std::uint32_t manager, std::uint32_t frame) override;
    void UpdateUI(std::uint32_t manager, double delta) override;
    std::uint32_t RenderEffectTransform(std::uint32_t manager, double delta) override;
    void CleanupEffects(std::uint32_t manager) override;
private:
    void CheckManager(std::uint32_t manager) const;
    void CheckCollection(std::uint32_t collection) const;
    std::uint32_t manager_;
    std::uint32_t& console_;
    std::filesystem::path root_;
    NativeCameraManagerOwner& cameras_;
    NativeUiCollectionScene& scene_;
    NativeUiCameraSnapshot& snapshot_;
    NativeUiFrameUpdate& frames_;
    NativeEffectSceneSession& effects_;
    NativeSystemMenuServices services_;
};
}  // namespace pal4
