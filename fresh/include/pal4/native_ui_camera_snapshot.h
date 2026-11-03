#pragma once

#include "pal4/renderware_image_native.h"
#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {
class NativeCameraManagerOwner;

// Borrows a live frame-manager image and initialized camera manager. Owns the
// cached SDK image at manager+148h; release before either manager or RW stops.
class NativeUiCameraSnapshot final : private IUiFrameManagerCameraRuntime {
public:
    NativeUiCameraSnapshot(std::uint32_t manager, NativeCameraManagerOwner& cameras);
    ~NativeUiCameraSnapshot();
    NativeUiCameraSnapshot(const NativeUiCameraSnapshot&) = delete;
    NativeUiCameraSnapshot& operator=(const NativeUiCameraSnapshot&) = delete;
    // Original 4BFE30 return: image on copy, camera on missing raster, zero on
    // missing camera. CachedImage is the authoritative image, not this return.
    [[nodiscard]] std::uint32_t Capture();
    [[nodiscard]] std::uint32_t CachedImage() const noexcept;
    void Release() noexcept;

private:
    std::uint32_t InitializeCameraSubsystem() override;
    std::uint32_t ActiveCameraObject(std::uint32_t subsystem) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t manager_;
    NativeCameraManagerOwner& cameras_;
    NativeRenderWareImageApi images_;
};
}  // namespace pal4
