#include "pal4/game_save_thumbnail.h"

#include <cstring>
#include <string>

namespace pal4 {

std::uint8_t GameSaveThumbnail::Capture(
    const std::int32_t slot, IGameSaveThumbnailRuntime& runtime) {
    const auto first_manager = runtime.PackedFileManager();
    runtime.ProcessPackedFile3(first_manager);
    const auto image = runtime.CreateImage(kWidth, kHeight, kDepth);
    runtime.AllocateImagePixels(image);

    const auto ui = runtime.UiFrameManager();
    const auto source = runtime.FrameBuffer(ui + kFrameBufferOffset);
    const auto destination = runtime.ImageBuffer(image);
    const auto x_step = source.width / kWidth;
    const auto y_step = source.height / kHeight;
    for (std::uint32_t y = 0; y < kHeight; ++y) {
        for (std::uint32_t x = 0; x < kWidth; ++x) {
            const auto source_offset =
                static_cast<std::size_t>(y) * y_step * source.stride +
                static_cast<std::size_t>(x) * x_step * 4u;
            const auto destination_offset =
                static_cast<std::size_t>(y) * destination.stride +
                static_cast<std::size_t>(x) * 4u;
            std::memcpy(destination.pixels.data() + destination_offset,
                        source.pixels.data() + source_offset, 4u);
            destination.pixels[destination_offset + 3u] = 0xffu;
        }
    }

    const auto filename = "save\\" + std::to_string(slot) + ".png";
    runtime.DeleteFile(filename);
    runtime.SaveImage(image, filename);
    const auto dictionary = runtime.TextureDictionary();
    runtime.RemoveTextureByCrc32(dictionary, filename);
    runtime.DestroyImage(image);
    const auto second_manager = runtime.PackedFileManager();
    runtime.ProcessPackedFile4(second_manager);
    return 1u;
}

}  // namespace pal4
