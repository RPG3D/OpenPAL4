#include "pal4/rw_texture_image_factory.h"

namespace pal4 {

std::uint32_t RwTextureImageFactory::CreateFromTexture(
    const std::uint32_t texture, const float scale,
    IRwTextureImageFactoryRuntime& runtime) {
    if (texture == 0) return 0;
    const auto source_raster = runtime.ReadWord(texture);
    if (source_raster == 0) return 0;

    const auto width = runtime.ReadWord(source_raster + 12u);
    const auto height = runtime.ReadWord(source_raster + 16u);
    const auto image = runtime.CreateImage(width, height, 32u);
    if (image == 0) return 0;
    runtime.AllocatePixels(image);
    runtime.SetImageFromRaster(image, source_raster);

    const auto source_flags = runtime.ReadWord(texture + 80u);
    const auto use_alpha =
        (source_flags & 0xf00u) == 0x300u ||
        (source_flags & 0xf000u) == 0x3000u;
    const auto format =
        static_cast<std::uint32_t>(runtime.ReadByte(source_raster + 32u) & 7u) |
        ((static_cast<std::uint32_t>(
              runtime.ReadByte(source_raster + 35u) & 0x80u) | 6u) << 8u);
    const auto new_raster = RwNormalMapRasterBuilder::Create(
        image, format, use_alpha, scale, runtime);
    const auto result = new_raster != 0
                            ? runtime.CreateTexture(new_raster) : 0u;
    if (result != 0) {
        const auto target_flags = runtime.ReadWord(result + 80u);
        runtime.WriteWord(result + 80u,
                          (target_flags & 0xffff0000u) |
                              (source_flags & 0xffffu));
    }
    runtime.DestroyImage(image);
    return result;
}

}  // namespace pal4
