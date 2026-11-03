#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct EffectRenderParameters {
    std::uint32_t texture{};
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t padding{};
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};
    float elapsed{};
    float duration{};
    float alpha{};
    float alpha_start{};
    float alpha_end{};
    float alpha_on_finish{};
    std::uint32_t phase{};
    std::uint32_t keep_active{};
    std::uint32_t active{};
};
static_assert(sizeof(EffectRenderParameters) == 60u);
static_assert(offsetof(EffectRenderParameters, elapsed) == 24u);
static_assert(offsetof(EffectRenderParameters, alpha) == 32u);
static_assert(offsetof(EffectRenderParameters, active) == 56u);

class IEffectRenderRuntime {
public:
    virtual ~IEffectRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t DestroyTexture(
        std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t TextureDictionary() = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t dictionary, std::uint32_t width,
        std::uint32_t height, std::uint32_t format,
        std::uint32_t color) = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(float value) = 0;
    [[nodiscard]] virtual std::uint32_t DrawTexturedRectangle(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height, std::uint32_t texture,
        std::uint8_t red, std::uint8_t green, std::uint8_t blue,
        std::int32_t alpha) = 0;
    [[nodiscard]] virtual float Interpolate(
        float start, float end, float progress, std::uint32_t mode) = 0;
};

class EffectRenderRuntimeStub final : public IEffectRenderRuntime {
public:
    [[nodiscard]] std::uint32_t DestroyTexture(
        std::uint32_t texture) override;
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
        float start, float end, float progress, std::uint32_t mode) override;
};

class EffectRenderControl final {
public:
    [[nodiscard]] static EffectRenderParameters& Initialize(
        EffectRenderParameters& state, std::int32_t screen_width,
        std::int32_t screen_height);
    [[nodiscard]] static std::uint32_t DestroyTexture(
        EffectRenderParameters& state, IEffectRenderRuntime& runtime);
    static void ResetAnimation(EffectRenderParameters& state);
    static void InitializeTexture(
        EffectRenderParameters& state, IEffectRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t Draw(
        const EffectRenderParameters& state, std::uint32_t context,
        IEffectRenderRuntime& runtime);
    static void AdvanceFade(
        EffectRenderParameters& state, float delta,
        IEffectRenderRuntime& runtime);
};

}  // namespace pal4
