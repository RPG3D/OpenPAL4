#pragma once

#include <cstdint>

struct RpWorld;
struct RwCamera;
struct RwFrame;
struct RwRaster;

namespace pal4 {

// Owns the SDK camera resources while an optional UI world is attached.
class NativeUiWorldCamera final {
public:
    NativeUiWorldCamera() = default;
    ~NativeUiWorldCamera();
    NativeUiWorldCamera(const NativeUiWorldCamera&) = delete;
    NativeUiWorldCamera& operator=(const NativeUiWorldCamera&) = delete;

    [[nodiscard]] bool Open(RpWorld* world, std::uint32_t width,
                            std::uint32_t height,
                            bool display_raster = false);
    void Close() noexcept;
    [[nodiscard]] RwCamera* Camera() const noexcept { return camera_; }
    [[nodiscard]] RwRaster* ColorRaster() const noexcept { return color_; }

private:
    RpWorld* world_ = nullptr;
    RwCamera* camera_ = nullptr;
    RwFrame* frame_ = nullptr;
    RwRaster* color_ = nullptr;
    RwRaster* depth_ = nullptr;
};

}  // namespace pal4
