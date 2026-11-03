#pragma once

#include "pal4/cegui_rectangle_queue.h"

#include <cstdint>
#include <map>

namespace pal4 {

struct CeguiRenderVertex {
    float x = 0.0f;
    float y = 0.0f;
    float depth = 0.0f;
    float reciprocal_camera_depth = 0.0f;
    std::uint32_t colour = 0u;
    float u = 0.0f;
    float v = 0.0f;
};
static_assert(sizeof(CeguiRenderVertex) == 28u);

class ICeguiVertexRuntime {
public:
    virtual ~ICeguiVertexRuntime() = default;
    virtual void SetRenderStates() = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state_id) = 0;
    [[nodiscard]] virtual std::uint32_t SetRenderState(
        std::uint32_t state_id, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ActiveCameraDepth() = 0;
    [[nodiscard]] virtual const CeguiRectangleRecord& Record(
        std::uint32_t address) = 0;
    virtual void WriteVertex(std::uint32_t address,
                             const CeguiRenderVertex& vertex) = 0;
    [[nodiscard]] virtual std::uint32_t Flush(
        CeguiRendererState& state) = 0;
    virtual void BindTexture(std::uint32_t texture_address) = 0;
    virtual void FinishNoOp(CeguiRendererState& state) = 0;
};

class CeguiVertexRuntimeStub final : public ICeguiVertexRuntime {
public:
    explicit CeguiVertexRuntimeStub(const CeguiRectangleQueueStub& queue)
        : queue_(queue) {}
    void SetRenderStates() override;
    [[nodiscard]] std::uint32_t GetRenderState(
        std::uint32_t state_id) override;
    [[nodiscard]] std::uint32_t SetRenderState(
        std::uint32_t state_id, std::uint32_t value) override;
    [[nodiscard]] float ActiveCameraDepth() override;
    [[nodiscard]] const CeguiRectangleRecord& Record(
        std::uint32_t address) override;
    void WriteVertex(std::uint32_t address,
                     const CeguiRenderVertex& vertex) override;
    [[nodiscard]] std::uint32_t Flush(CeguiRendererState& state) override;
    void BindTexture(std::uint32_t texture_address) override;
    void FinishNoOp(CeguiRendererState& state) override;
    [[nodiscard]] const std::map<std::uint32_t, CeguiRenderVertex>& Vertices()
        const noexcept { return vertices_; }

private:
    const CeguiRectangleQueueStub& queue_;
    std::map<std::uint32_t, CeguiRenderVertex> vertices_;
};

class CeguiVertexProcessor final {
public:
    [[nodiscard]] static std::uint32_t ProcessScaled(
        CeguiRendererState& state, std::int32_t config_width,
        std::int32_t config_height, ICeguiVertexRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessNative(
        CeguiRendererState& state, ICeguiVertexRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessRendererScaled(
        CeguiRendererState& state, ICeguiVertexRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessWide(
        CeguiRendererState& state, ICeguiVertexRuntime& runtime);
};

}  // namespace pal4
