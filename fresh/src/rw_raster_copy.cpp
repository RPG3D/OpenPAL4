#include "pal4/rw_raster_copy.h"

#include <cstring>

namespace pal4 {

std::uint32_t RwRasterCopy::LockAndCopyData(
    const std::uint32_t texture, const void* pixels,
    const std::int32_t x, const std::int32_t y,
    const std::int32_t width, const std::int32_t height,
    IRwRasterCopyRuntime& runtime) {
    if (texture == 0 || pixels == nullptr) return 0;
    const auto raster = runtime.TextureRaster(texture);
    if (width + x > runtime.RasterWidth(raster) ||
        height + y > runtime.RasterHeight(raster))
        return 0;

    auto* destination = runtime.LockRaster(raster, 0, 1);
    if (destination == nullptr) return 0;
    const auto bytes_per_pixel = runtime.RasterDepthBits(raster) / 8;
    const auto stride = runtime.RasterStride(raster);
    auto* source = static_cast<const std::uint8_t*>(pixels);
    destination += x * bytes_per_pixel + y * stride;
    for (std::int32_t row = 0; row < height; ++row) {
        if (bytes_per_pixel > 0 && width > 0)
            std::memcpy(destination,
                        source + row * width * bytes_per_pixel,
                        static_cast<std::size_t>(width) * bytes_per_pixel);
        destination += stride;
    }
    runtime.UnlockRaster(raster);
    return texture;
}

}  // namespace pal4
