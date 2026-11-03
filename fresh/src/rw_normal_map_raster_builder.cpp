#include "pal4/rw_normal_map_raster_builder.h"

#include <algorithm>

namespace pal4 {

std::uint32_t RwNormalMapRasterBuilder::Create(
    const std::uint32_t image, const std::uint32_t format,
    const bool use_alpha, float scale,
    IRwNormalMapRasterRuntime& runtime) {
    if (image == 0) return 0;
    const auto image_width = runtime.ReadWord(image + 4u);
    const auto image_height = runtime.ReadWord(image + 8u);
    const auto raster = runtime.CreateRaster(
        image_width, image_height, 32u, format);
    if (raster == 0) return 0;

    auto width = runtime.ReadWord(raster + 12u);
    auto height = runtime.ReadWord(raster + 16u);
    const auto levels = runtime.RasterLevelCount(raster);
    for (std::uint32_t level = 0; level < levels; ++level) {
        runtime.LockRaster(raster, level, 5u);
        std::uint32_t normal_image = 0;
        if (runtime.ReadWord(image + 4u) == width &&
            runtime.ReadWord(image + 8u) == height) {
            normal_image = runtime.CreateNormalMapImage(
                image, use_alpha, scale);
        } else {
            const auto resampled = runtime.ResampleImage(
                image, width, height);
            if (resampled != 0) {
                normal_image = runtime.CreateNormalMapImage(
                    resampled, use_alpha, scale);
                runtime.DestroyImage(resampled);
            }
        }
        if (normal_image != 0) {
            runtime.SetRasterFromImage(raster, normal_image);
            runtime.DestroyImage(normal_image);
        }
        runtime.UnlockRaster(raster);
        width = std::max(1u, width / 2u);
        height = std::max(1u, height / 2u);
        scale *= 0.5f;
    }
    return raster;
}

}  // namespace pal4
