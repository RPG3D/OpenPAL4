#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CollisionRenderContext final {
    std::array<float, 3u> camera_position{};
    std::uint32_t source_matrix = 0u;
    std::array<float, 16u> screen_matrix{};
    std::uint8_t intensity = 0u;
    std::array<std::uint8_t, 3u> padding{};
    std::uint32_t render_mode = 0u;
    std::uint32_t flush_count = 0u;
    std::uint32_t intersection = 0u;
};

static_assert(sizeof(CollisionRenderContext) == 96);
static_assert(offsetof(CollisionRenderContext, screen_matrix) == 16);
static_assert(offsetof(CollisionRenderContext, intensity) == 80);
static_assert(offsetof(CollisionRenderContext, render_mode) == 84);
static_assert(offsetof(CollisionRenderContext, flush_count) == 88);

struct CollisionImmediateVertex final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    std::array<std::uint32_t, 3u> native_tail{};
    std::uint32_t color = 0u;
    float u = 0.0f;
    float v = 0.0f;
};

static_assert(sizeof(CollisionImmediateVertex) == 36);
static_assert(offsetof(CollisionImmediateVertex, color) == 24);
static_assert(offsetof(CollisionImmediateVertex, u) == 28);

class ICameraCollisionRenderRuntime {
public:
    virtual ~ICameraCollisionRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    [[nodiscard]] virtual std::uint32_t SetDeviceState(
        std::uint32_t state, std::uint32_t value) = 0;
    virtual void SetupTransformMatrix(
        std::array<float, 16u>& destination,
        std::uint32_t source) = 0;
    virtual void ScaleMatrix(
        std::array<float, 16u>& matrix,
        const std::array<float, 3u>& scale,
        std::uint32_t combine_mode) = 0;
    virtual void TranslateMatrix(
        std::array<float, 16u>& matrix,
        const std::array<float, 3u>& offset,
        std::uint32_t combine_mode) = 0;
    virtual void TraverseIntersections(
        std::uint32_t world, std::uint32_t intersection,
        std::uint32_t callback, CollisionRenderContext& context) = 0;
    [[nodiscard]] virtual std::uint32_t VertexCount() = 0;
    virtual void SetVertexCount(std::uint32_t count) = 0;
    virtual void SetStatistic(std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t TransformVertices(
        std::uint32_t count, std::uint32_t matrix,
        std::uint32_t flags) = 0;
    virtual void RenderPrimitive(std::uint32_t primitive) = 0;
    virtual void EndPrimitive() = 0;
    virtual void TransformPoints(
        std::array<std::array<float, 3u>, 3u>& destination,
        const std::array<std::array<float, 3u>, 3u>& source,
        const std::array<float, 16u>& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    virtual void SetRenderState(
        std::uint32_t state, std::uint32_t value) = 0;
    virtual void WriteVertex(
        std::uint32_t index, const CollisionImmediateVertex& vertex) = 0;
};

class CameraCollisionRenderRuntimeStub final :
    public ICameraCollisionRenderRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::int32_t Ftol(double) override { return 0; }
    [[nodiscard]] std::uint32_t SetDeviceState(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void SetupTransformMatrix(
        std::array<float, 16u>&, std::uint32_t) override {}
    void ScaleMatrix(std::array<float, 16u>&,
                     const std::array<float, 3u>&,
                     std::uint32_t) override {}
    void TranslateMatrix(std::array<float, 16u>&,
                         const std::array<float, 3u>&,
                         std::uint32_t) override {}
    void TraverseIntersections(
        std::uint32_t, std::uint32_t, std::uint32_t,
        CollisionRenderContext&) override {}
    [[nodiscard]] std::uint32_t VertexCount() override { return 0u; }
    void SetVertexCount(std::uint32_t) override {}
    void SetStatistic(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t TransformVertices(
        std::uint32_t, std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    void RenderPrimitive(std::uint32_t) override {}
    void EndPrimitive() override {}
    void TransformPoints(
        std::array<std::array<float, 3u>, 3u>&,
        const std::array<std::array<float, 3u>, 3u>&,
        const std::array<float, 16u>&) override {}
    [[nodiscard]] std::uint32_t GetRenderState(
        std::uint32_t) override { return 0u; }
    void SetRenderState(std::uint32_t, std::uint32_t) override {}
    void WriteVertex(std::uint32_t,
                     const CollisionImmediateVertex&) override {}
};

class CameraCollisionRender final {
public:
    // IDA 0x5fe040.
    [[nodiscard]] static std::uint32_t RenderWorld(
        std::uint32_t camera, std::uint32_t texture,
        std::uint32_t world, std::uint32_t intersection,
        float intensity, std::uint32_t unused,
        std::uint32_t render_mode,
        ICameraCollisionRenderRuntime& runtime);
    // IDA 0x5fe270.
    [[nodiscard]] static std::uint32_t RenderGeometry(
        std::uint32_t intersection, std::uint32_t sector,
        std::uint32_t triangle, float distance,
        CollisionRenderContext& context,
        ICameraCollisionRenderRuntime& runtime);
};

}  // namespace pal4
