#include "pal4/png_image_info.h"

#include <stdexcept>

namespace pal4 {

PngImageInfo* PngImageInfoWriter::SetGamma(
    void* context, PngImageInfo* info, double gamma) {
    if (!context || !info) return nullptr;
    info->valid |= 1;
    info->gamma = static_cast<float>(gamma);
    return info;
}

void PngImageInfoWriter::SetImage(
    void* context, PngImageInfo* info, std::uint32_t width,
    std::uint32_t height, std::uint8_t bit_depth, std::uint8_t color_type,
    std::uint8_t interlace, std::uint8_t compression, std::uint8_t filter,
    IPngImageInfoErrors& errors) {
    if (!context || !info) return;
    info->compression = compression;
    info->height = height;
    info->filter = filter;
    info->width = width;
    info->bit_depth = bit_depth;
    info->color_type = color_type;
    info->interlace = interlace;
    info->channels = (color_type == 3 || (color_type & 2) == 0) ? 1 : 3;
    if (color_type & 4) ++info->channels;
    info->pixel_depth = static_cast<std::uint8_t>(bit_depth * info->channels);

    const auto bytes_per_pixel = (static_cast<unsigned>(info->pixel_depth) + 7) >> 3;
    if (bytes_per_pixel == 0)
        throw std::domain_error("PNG pixel depth produces a zero rowbyte divisor");
    if (width > 0x7fffffffu / bytes_per_pixel) {
        errors.Emit(context,
                    "Width too large to process image data; rowbytes will overflow.");
        info->rowbytes = 0;
    } else {
        info->rowbytes = (width * static_cast<std::uint32_t>(info->pixel_depth) + 7u) >> 3;
    }
}

void PngImageInfoWriter::SetBackground(
    void* context, PngImageInfo* info, std::uint32_t background,
    std::uint16_t index) {
    if (!context || !info) return;
    info->valid |= 8;
    info->background = background;
    info->background_index = index;
}

void PngImageInfoWriter::SetSrgbIntent(
    void* context, PngImageInfo* info, std::uint8_t intent) {
    if (!context || !info) return;
    info->valid |= 0x800;
    info->srgb_intent = intent;
}

PngImageInfo* PngImageInfoWriter::SetSrgbGamma(
    void* context, PngImageInfo* info, std::uint8_t intent) {
    if (!context || !info) return nullptr;
    SetSrgbIntent(context, info, intent);
    return SetGamma(context, info, 0.4545499980449677);
}

void PngImageInfoWriter::SetTransparency(
    void* context, PngImageInfo* info, std::uint32_t palette_address,
    std::uint32_t count, const std::array<std::uint8_t, 10>* color) {
    if (!context || !info) return;
    if (palette_address) info->transparency_palette_address = palette_address;
    if (color) {
        info->transparency_color = *color;
        if (!count) count = 1;
    }
    info->valid |= 0x10;
    info->transparency_count = static_cast<std::uint16_t>(count);
}

}  // namespace pal4
