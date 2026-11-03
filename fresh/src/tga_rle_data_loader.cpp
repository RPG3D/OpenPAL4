#include "pal4/tga_rle_data_loader.h"

#include <array>

namespace pal4 {

std::uint32_t TgaRleDataLoader::Load(const std::uint32_t stream,
                                     const std::uint32_t destination,
                                     ITgaRleRuntime& runtime) {
    const auto handle = runtime.ReadWord(stream);
    const auto width = static_cast<std::uint16_t>(
        runtime.ReadByte(stream + 26u) |
        (static_cast<std::uint16_t>(runtime.ReadByte(stream + 27u)) << 8u));
    const auto bytes_per_pixel =
        static_cast<std::uint32_t>(runtime.ReadByte(stream + 30u) >> 3u);
    if (destination == 0u) return 1u;

    std::array<std::uint8_t, 32u> pixel{};
    std::uint8_t repeat_remaining = 0u;
    std::uint8_t literal_remaining = 0u;
    for (std::uint32_t column = 0u; column < width; ++column) {
        if (repeat_remaining == 0u && literal_remaining == 0u) {
            std::uint8_t control = 0u;
            if (runtime.ReadItem(handle, std::span(&control, 1u)) < 1u)
                return 1u;
            if (control < 0x80u) {
                literal_remaining = static_cast<std::uint8_t>(control + 1u);
            } else {
                repeat_remaining = static_cast<std::uint8_t>(control - 127u);
                if (runtime.ReadItem(handle,
                        std::span(pixel.data(), bytes_per_pixel)) < 1u)
                    return 1u;
            }
        }
        if (repeat_remaining != 0u) {
            --repeat_remaining;
        } else {
            if (runtime.ReadItem(handle,
                    std::span(pixel.data(), bytes_per_pixel)) < 1u)
                return 1u;
            --literal_remaining;
        }
        for (std::uint32_t channel = 0u; channel < bytes_per_pixel; ++channel)
            runtime.WriteByte(destination + column * bytes_per_pixel + channel,
                              pixel[channel]);
    }
    runtime.WriteWord(stream + 8u, 0u);
    return 0u;
}

}  // namespace pal4
