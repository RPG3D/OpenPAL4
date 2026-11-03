#pragma once

#include "pal4/dynamic_strip_object.h"
#include "pal4/dynamic_strip_object_update.h"

#include <cstdint>
#include <vector>

namespace pal4 {

class IDynamicStripObjectRenderRuntime {
public:
    virtual ~IDynamicStripObjectRenderRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t transform) = 0;
    virtual void RenderVisual(const DynamicStripObjectState& state) = 0;
};

class DynamicStripObjectRenderRuntimeStub final
    : public IDynamicStripObjectRenderRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectManager() override { return 0u; }
    [[nodiscard]] std::uint8_t ValidateTransform(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    void RenderVisual(const DynamicStripObjectState&) override {}
};

struct DynamicStripIm3DVertex final {
    float x{}, y{}, z{};
    float nx{}, ny{}, nz{};
    std::uint32_t color{};
    float u{}, v{};
};
static_assert(sizeof(DynamicStripIm3DVertex) == 36u);

struct DynamicStripBufferRange final {
    std::uint32_t begin{}, end{}, capacity{};
};

class IDynamicStripBufferRuntime {
public:
    virtual ~IDynamicStripBufferRuntime() = default;
    virtual void Free(std::uint32_t address) = 0;
};

class DynamicStripVisualBuffers final {
public:
    // IDA 0x64a080/0x64a0a0, CRT atexit callbacks.
    static void CleanupIndices(DynamicStripBufferRange& range,
                               IDynamicStripBufferRuntime& runtime);
    static void CleanupVertices(DynamicStripBufferRange& range,
                                IDynamicStripBufferRuntime& runtime);
};

class IDynamicStripVisualRenderRuntime {
public:
    virtual ~IDynamicStripVisualRenderRuntime() = default;
    [[nodiscard]] virtual std::vector<DynamicStripSegment> Segments(
        std::uint32_t sentinel) = 0;
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
    virtual void Transform(const std::vector<DynamicStripIm3DVertex>& vertices,
                           std::uint32_t flags) = 0;
    virtual void RenderIndexed(std::uint32_t primitive,
                               const std::vector<std::uint16_t>& indices) = 0;
    virtual void End() = 0;
};

class DynamicStripVisualRenderRuntimeStub final
    : public IDynamicStripVisualRenderRuntime {
public:
    [[nodiscard]] std::vector<DynamicStripSegment> Segments(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t TextureRaster(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetRenderState(
        std::uint32_t) override { return 0u; }
    void SetRenderState(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GetDeviceState(
        std::uint32_t) override { return 0u; }
    void SetDeviceState(std::uint32_t, std::uint32_t) override {}
    void Transform(const std::vector<DynamicStripIm3DVertex>&,
                   std::uint32_t) override {}
    void RenderIndexed(std::uint32_t,
                       const std::vector<std::uint16_t>&) override {}
    void End() override {}
};

class DynamicStripObjectRenderer final {
public:
    // IDA 0x6499e0.
    static void RenderVisual(const DynamicStripObjectState& state,
                             IDynamicStripVisualRenderRuntime& runtime);
    static void RenderVisual(StripVisualBlockConstView state,
                             IDynamicStripVisualRenderRuntime& runtime);
    // IDA 0x60f1a0.
    static void Render(const DynamicStripObjectState& state,
                       std::uint32_t context,
                       IDynamicStripObjectRenderRuntime& runtime);
};

}  // namespace pal4
