#pragma once

#include "pal4/effect_render_parameters.h"

namespace pal4 {

// RenderWare SDK adapter for the existing PAL4 screen-effect model.
// Draw context is a pointer to the active RwCamera* slot, as in the game ABI.
class NativeEffectRenderRuntime final : public IEffectRenderRuntime {
public:
    [[nodiscard]] std::uint32_t DestroyTexture(std::uint32_t texture) override;
    [[nodiscard]] std::uint32_t TextureDictionary() override;
    [[nodiscard]] std::uint32_t CreateTexture(
        std::uint32_t dictionary, std::uint32_t width,
        std::uint32_t height, std::uint32_t format,
        std::uint32_t color) override;
    [[nodiscard]] std::int32_t FloatToLong(float value) override;
    [[nodiscard]] std::uint32_t DrawTexturedRectangle(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height, std::uint32_t texture,
        std::uint8_t red, std::uint8_t green, std::uint8_t blue,
        std::int32_t alpha) override;
    [[nodiscard]] float Interpolate(
        float start, float end, float progress,
        std::uint32_t mode) override;
};

}  // namespace pal4
