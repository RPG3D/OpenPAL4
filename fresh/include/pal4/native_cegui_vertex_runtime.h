#pragma once

#include "pal4/cegui_render_helpers.h"
#include "pal4/cegui_vertex_processor.h"

namespace pal4 {

class NativeCeguiVertexRuntime final : public ICeguiVertexRuntime,
                                       public ICeguiRenderHelperRuntime {
public:
    void SetRenderStates() override;
    [[nodiscard]] std::uint32_t GetRenderState(std::uint32_t state_id) override;
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

    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t DrawIndexed(
        std::uint32_t primitive_type, std::uint32_t vertices,
        std::uint32_t vertex_count, std::uint32_t indices,
        std::uint32_t index_count) override;

private:
    CeguiRectangleRecord record_;
};

}  // namespace pal4
