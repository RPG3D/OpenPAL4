#pragma once
#include "pal4/rw_raster_copy.h"
namespace pal4 {
class NativeRwRasterCopyRuntime final : public IRwRasterCopyRuntime {
public:
    std::uint32_t TextureRaster(std::uint32_t texture) override;
    std::int32_t RasterWidth(std::uint32_t raster) override;
    std::int32_t RasterHeight(std::uint32_t raster) override;
    std::int32_t RasterDepthBits(std::uint32_t raster) override;
    std::int32_t RasterStride(std::uint32_t raster) override;
    std::uint8_t* LockRaster(std::uint32_t raster, std::int32_t level, std::int32_t mode) override;
    void UnlockRaster(std::uint32_t raster) override;
};
}
