#include "pal4/tga_image_loader.h"

#include <new>

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\Common\\TgaLoader\\RwTgaLoader.c";

std::uint16_t ReadHalfWord(const std::uint32_t address,
                           ITgaImageLoadRuntime& runtime) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(runtime.ReadByte(address)) |
        (static_cast<std::uint16_t>(runtime.ReadByte(address + 1u)) << 8u));
}

void CopyPixels(const std::uint32_t stream,
                const std::uint32_t metadata,
                const std::uint32_t image,
                ITgaImageLoadRuntime& runtime) {
    const auto width = ReadHalfWord(stream + 26u, runtime);
    const auto height = ReadHalfWord(stream + 28u, runtime);
    const auto stride = runtime.ReadWord(image + 16u);
    const auto pixels = runtime.ReadWord(image + 20u);
    const auto source = runtime.ReadWord(metadata + 8u);
    const bool top_down = runtime.ReadByte(stream + 33u) != 0u;
    for (std::uint32_t row = 0u; row < height; ++row) {
        const auto destination_row = top_down ? row : height - 1u - row;
        const auto row_address = pixels + stride * destination_row;
        for (std::uint32_t column = 0u; column < width; ++column) {
            const auto source_pixel = source + (row * width + column) * 4u;
            const auto destination_pixel = row_address + column * 4u;
            for (std::uint32_t channel = 0u; channel < 4u; ++channel)
                runtime.WriteByte(destination_pixel + channel,
                                  runtime.ReadByte(source_pixel + channel));
        }
    }
}

}  // namespace

std::uint32_t TgaImageLoader::Load(const std::uint32_t source,
                                    ITgaImageLoadRuntime& runtime) {
    const auto stream = runtime.OpenFile(source, "r");
    if (stream == 0u || runtime.ReadWord(stream + 8u) != 0u) return 0u;

    const auto metadata = runtime.AllocateMetadata(16u, 0u);
    if (metadata == 0u) throw std::bad_alloc{};
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        runtime.WriteWord(metadata + offset, 0u);
    runtime.WriteWord(metadata + 12u, 47u);

    if (runtime.LoadHeader(stream, metadata)) {
        runtime.CloseFile(stream);
        (void)FileStreamLifecycle::FreeMembers(metadata, runtime);
        runtime.NotifyError(stream, 5u);
        runtime.LogError(kSourceFile, 66u, runtime.ErrorMessage(5u));
        runtime.WriteWord(stream + 8u, 5u);
        return 0u;
    }

    std::uint32_t image = 0u;
    if ((runtime.ReadByte(metadata + 12u) & 4u) != 0u &&
        runtime.ReadByte(stream + 14u) == 2u) {
        const auto width = ReadHalfWord(stream + 26u, runtime);
        const auto height = ReadHalfWord(stream + 28u, runtime);
        image = runtime.CreateImage(width, height, 32u);
        if (image != 0u) {
            runtime.AllocatePixels(image);
            CopyPixels(stream, metadata, image, runtime);
        }
    }
    runtime.CloseFile(stream);
    (void)FileStreamLifecycle::FreeMembers(metadata, runtime);
    return image;
}

}  // namespace pal4
