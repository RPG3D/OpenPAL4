#include "pal4/cegui_vertex_processor.h"

namespace pal4 {

void CeguiVertexRuntimeStub::SetRenderStates() {}

std::uint32_t CeguiVertexRuntimeStub::GetRenderState(
    std::uint32_t /*state_id*/) { return 0u; }

std::uint32_t CeguiVertexRuntimeStub::SetRenderState(
    std::uint32_t /*state_id*/, std::uint32_t /*value*/) { return 0u; }

float CeguiVertexRuntimeStub::ActiveCameraDepth() { return 1.0f; }

const CeguiRectangleRecord& CeguiVertexRuntimeStub::Record(
    const std::uint32_t address) {
    return queue_.Record(address);
}

void CeguiVertexRuntimeStub::WriteVertex(
    const std::uint32_t address, const CeguiRenderVertex& vertex) {
    vertices_[address] = vertex;
}

std::uint32_t CeguiVertexRuntimeStub::Flush(CeguiRendererState& state) {
    state.WriteDword(264u, 0u);
    return 0u;
}

void CeguiVertexRuntimeStub::BindTexture(
    std::uint32_t /*texture_address*/) {}

void CeguiVertexRuntimeStub::FinishNoOp(
    CeguiRendererState& /*state*/) {}

}  // namespace pal4
