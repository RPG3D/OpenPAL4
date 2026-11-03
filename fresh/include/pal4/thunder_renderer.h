#pragma once

#include "pal4/thunder_object.h"

#include <cstdint>
#include <array>
#include <vector>

namespace pal4 {

class IThunderRenderRuntime {
public:
    virtual ~IThunderRenderRuntime() = default;
    [[nodiscard]] virtual bool OwnerVisible(std::uint32_t owner,
                                             std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t resource) = 0;
    [[nodiscard]] virtual bool EffectManagerAvailable() = 0;
    [[nodiscard]] virtual bool ValidateTransform(
        std::uint32_t context, std::uint32_t transform) = 0;
    virtual void RenderVisual(ThunderObjectState& state) = 0;
};

class ThunderRenderRuntimeStub final : public IThunderRenderRuntime {
public:
    [[nodiscard]] bool OwnerVisible(std::uint32_t,
                                     std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t ObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] bool EffectManagerAvailable() override { return false; }
    [[nodiscard]] bool ValidateTransform(
        std::uint32_t, std::uint32_t) override { return false; }
    void RenderVisual(ThunderObjectState&) override {}
};

struct ThunderVisualSegment final {
    std::array<float, 3u> first{};
    std::array<float, 3u> second{};
};
static_assert(sizeof(ThunderVisualSegment) == 24u);

struct ThunderIm3DVertex final {
    float x{}, y{}, z{};
    float nx{}, ny{}, nz{};
    std::uint32_t color{};
    float u{}, v{};
};
static_assert(sizeof(ThunderIm3DVertex) == 36u);

class IThunderVisualRenderRuntime {
public:
    virtual ~IThunderVisualRenderRuntime() = default;
    virtual void UpdateTransformProperties(ThunderObjectState& state) = 0;
    virtual void UpdateVisual(ThunderObjectState& state) = 0;
    virtual void UpdateCollection(ThunderObjectState& state) = 0;
    [[nodiscard]] virtual std::vector<ThunderVisualSegment> ReadSegments(
        std::uint32_t begin, std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t TextureRaster(
        std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetDeviceState(
        std::uint32_t state) = 0;
    virtual void SetDeviceState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void TransformQuad(const std::array<ThunderIm3DVertex, 4u>& vertices,
                               std::uint32_t flags) = 0;
    virtual void RenderIndexed(std::uint32_t primitive,
                               const std::array<std::uint16_t, 4u>& indices) = 0;
    virtual void End() = 0;
};

class ThunderVisualRenderRuntimeStub final
    : public IThunderVisualRenderRuntime {
public:
    void UpdateTransformProperties(ThunderObjectState&) override {}
    void UpdateVisual(ThunderObjectState&) override {}
    void UpdateCollection(ThunderObjectState&) override {}
    [[nodiscard]] std::vector<ThunderVisualSegment> ReadSegments(
        std::uint32_t, std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t TextureRaster(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetRenderState(
        std::uint32_t) override { return 0u; }
    void SetRenderState(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GetDeviceState(
        std::uint32_t) override { return 0u; }
    void SetDeviceState(std::uint32_t, std::uint32_t) override {}
    void TransformQuad(const std::array<ThunderIm3DVertex, 4u>&,
                       std::uint32_t) override {}
    void RenderIndexed(std::uint32_t,
                       const std::array<std::uint16_t, 4u>&) override {}
    void End() override {}
};

class ThunderRenderer final {
public:
    // IDA 0x64a450.
    static void RenderVisual(ThunderObjectState& state,
                             IThunderVisualRenderRuntime& runtime);

    // IDA 0x61c4a0.
    static void Render(ThunderObjectState& state, std::uint32_t context,
                       IThunderRenderRuntime& runtime);
};

}  // namespace pal4
