#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct PngImageInfo {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t valid = 0;
    std::uint32_t rowbytes = 0;
    std::uint32_t background = 0;
    std::uint16_t background_index = 0;
    std::uint16_t transparency_count = 0;
    std::uint8_t bit_depth = 0;
    std::uint8_t color_type = 0;
    std::uint8_t compression = 0;
    std::uint8_t filter = 0;
    std::uint8_t interlace = 0;
    std::uint8_t channels = 0;
    std::uint8_t pixel_depth = 0;
    std::uint8_t reserved31 = 0;
    std::array<std::uint8_t, 8> signature{};
    float gamma = 0;
    std::uint8_t srgb_intent = 0;
    std::uint8_t reserved45[3]{};
    std::uint32_t transparency_palette_address = 0;
    std::array<std::uint8_t, 10> transparency_color{};
    std::uint8_t reserved62[2]{};
};

static_assert(sizeof(PngImageInfo) == 64);
static_assert(offsetof(PngImageInfo, valid) == 8);
static_assert(offsetof(PngImageInfo, rowbytes) == 12);
static_assert(offsetof(PngImageInfo, bit_depth) == 24);
static_assert(offsetof(PngImageInfo, gamma) == 40);
static_assert(offsetof(PngImageInfo, srgb_intent) == 44);
static_assert(offsetof(PngImageInfo, transparency_count) == 22);
static_assert(offsetof(PngImageInfo, transparency_palette_address) == 48);
static_assert(offsetof(PngImageInfo, transparency_color) == 52);
static_assert(offsetof(PngImageInfo, signature) == 32);

class IPngImageInfoErrors {
public:
    virtual ~IPngImageInfoErrors() = default;
    virtual void Emit(void* context, const char* message) = 0;
};

class PngImageInfoWriter final {
public:
    static PngImageInfo* SetGamma(void* context, PngImageInfo* info, double gamma);
    static void SetImage(void* context, PngImageInfo* info,
                         std::uint32_t width, std::uint32_t height,
                         std::uint8_t bit_depth, std::uint8_t color_type,
                         std::uint8_t interlace, std::uint8_t compression,
                         std::uint8_t filter, IPngImageInfoErrors& errors);
    static void SetBackground(void* context, PngImageInfo* info,
                              std::uint32_t background, std::uint16_t index);
    static void SetSrgbIntent(void* context, PngImageInfo* info,
                              std::uint8_t intent);
    static PngImageInfo* SetSrgbGamma(void* context, PngImageInfo* info,
                                      std::uint8_t intent);
    static void SetTransparency(
        void* context, PngImageInfo* info,
        std::uint32_t palette_address, std::uint32_t count,
        const std::array<std::uint8_t, 10>* color);
};

}  // namespace pal4
