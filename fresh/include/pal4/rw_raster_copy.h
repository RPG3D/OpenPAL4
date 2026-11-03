#pragma once

#include <cstdint>

namespace pal4 {

class IRwRasterCopyRuntime {
public:
    virtual ~IRwRasterCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t TextureRaster(
        std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::int32_t RasterWidth(
        std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::int32_t RasterHeight(
        std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::int32_t RasterDepthBits(
        std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::int32_t RasterStride(
        std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint8_t* LockRaster(
        std::uint32_t raster, std::int32_t level,
        std::int32_t mode) = 0;
    virtual void UnlockRaster(std::uint32_t raster) = 0;
};

class RwRasterCopy final {
public:
    [[nodiscard]] static std::uint32_t LockAndCopyData(
        std::uint32_t texture, const void* pixels,
        std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height,
        IRwRasterCopyRuntime& runtime);
};

}  // namespace pal4
