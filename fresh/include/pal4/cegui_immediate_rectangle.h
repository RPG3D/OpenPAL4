#pragma once

#include "pal4/cegui_rectangle_queue.h"
#include "pal4/cegui_vertex_processor.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICeguiImmediateRectangleRuntime : public CeguiPackedColor::IFloatToInt {
public:
    virtual ~ICeguiImmediateRectangleRuntime() = default;
    [[nodiscard]] virtual float ActiveCameraDepth() = 0;
    [[nodiscard]] virtual float RenderDepth() = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderState(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t SetRenderState(
        std::uint32_t id, std::uint32_t value) = 0;
    virtual void BindTexture(std::uint32_t texture_handle) = 0;
    [[nodiscard]] virtual std::uint32_t DrawTriangleStrip(
        const std::array<CeguiRenderVertex, 4>& vertices) = 0;
};

class CeguiImmediateRectangleStub : public ICeguiImmediateRectangleRuntime {
public:
    [[nodiscard]] std::int32_t Convert(double value) override;
    [[nodiscard]] float ActiveCameraDepth() override;
    [[nodiscard]] float RenderDepth() override;
    [[nodiscard]] std::uint32_t GetRenderState(std::uint32_t id) override;
    [[nodiscard]] std::uint32_t SetRenderState(
        std::uint32_t id, std::uint32_t value) override;
    void BindTexture(std::uint32_t texture_handle) override;
    [[nodiscard]] std::uint32_t DrawTriangleStrip(
        const std::array<CeguiRenderVertex, 4>& vertices) override;
};

class CeguiImmediateRectangle final {
public:
    // Rect/UV records use the DLL order top,bottom,left,right. IDA414F72..
    // 41514F emits the LB,RB,LT,RT strip; this also preserves corner colours.
    // 0x414a70: the six-argument renderer slot contains only retn 18h.
    static void EmptyRendererSlot() noexcept;

    // 0x414a80. Only the rectangle, texture and colour arguments are read.
    [[nodiscard]] static std::uint32_t Draw(
        const CeguiRectWords& rectangle, std::uint32_t texture_handle,
        const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        ICeguiImmediateRectangleRuntime& runtime);

    // 0x414e80: renderer-scaled variant with the common state setup values.
    [[nodiscard]] static std::uint32_t DrawScaled(
        const CeguiRendererState& state,
        const CeguiRectWords& rectangle, std::uint32_t texture_handle,
        const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        ICeguiImmediateRectangleRuntime& runtime);

    // 0x415260: wide/offset variant without state restoration.
    [[nodiscard]] static std::uint32_t DrawOffset(
        const CeguiRendererState& state,
        const CeguiRectWords& rectangle, std::uint32_t texture_handle,
        const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        ICeguiImmediateRectangleRuntime& runtime);
};

}  // namespace pal4
