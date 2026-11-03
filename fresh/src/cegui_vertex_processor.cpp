#include "pal4/cegui_vertex_processor.h"

#include <bit>

namespace pal4 {

namespace {

float ReadFloat(const CeguiRectangleRecord& record,
                const std::uint32_t offset) {
    return std::bit_cast<float>(record.ReadDword(offset));
}

std::uint32_t RenderColour(const CeguiRectangleRecord& record,
                           const std::uint32_t offset) {
    const auto bytes = record.ReadDword(offset);
    return ((bytes >> 16u) & 0xffu) |
           (bytes & 0xff00u) |
           ((bytes & 0xffu) << 16u) |
           (bytes & 0xff000000u);
}

std::uint32_t ProcessVertices(CeguiRendererState& state,
                              const float width_scale,
                              const float height_scale,
                              const float horizontal_bias,
                              ICeguiVertexRuntime& runtime) {
    const auto vertex_begin = state.ReadDword(204u);
    const auto reciprocal_camera_depth =
        1.0f / runtime.ActiveCameraDepth();
    std::uint32_t last_texture = 0u;
    for (auto address = state.ReadDword(188u);
         address != state.ReadDword(192u); address += 60u) {
        const auto& record = runtime.Record(address);
        const auto texture = record.ReadDword(0u);
        if (texture != last_texture) {
            static_cast<void>(runtime.Flush(state));
            runtime.BindTexture(texture);
            last_texture = texture;
        }

        const auto left = ReadFloat(record, 12u);
        const auto right = ReadFloat(record, 16u);
        const auto top = ReadFloat(record, 4u);
        const auto bottom = ReadFloat(record, 8u);
        const auto depth = ReadFloat(record, 20u);
        const auto u0 = ReadFloat(record, 32u);
        const auto u1 = ReadFloat(record, 36u);
        const auto v0 = ReadFloat(record, 24u);
        const auto v1 = ReadFloat(record, 28u);
        const CeguiRenderVertex vertices[4]{
            {width_scale * left + horizontal_bias - 0.5f,
             height_scale * top - 0.5f, depth,
             reciprocal_camera_depth, RenderColour(record, 40u), u0, v0},
            {width_scale * left + horizontal_bias - 0.5f,
             height_scale * bottom - 0.5f, depth,
             reciprocal_camera_depth, RenderColour(record, 48u), u0, v1},
            {width_scale * right + horizontal_bias - 0.5f,
             height_scale * bottom - 0.5f, depth,
             reciprocal_camera_depth, RenderColour(record, 52u), u1, v1},
            {width_scale * right + horizontal_bias - 0.5f,
             height_scale * top - 0.5f, depth,
             reciprocal_camera_depth, RenderColour(record, 44u), u1, v0},
        };
        for (const auto& vertex : vertices) {
            const auto index = state.ReadDword(264u);
            runtime.WriteVertex(vertex_begin + 28u * index, vertex);
            state.WriteDword(264u, index + 1u);
        }
        if (state.ReadDword(264u) > 4092u) {
            static_cast<void>(runtime.Flush(state));
        }
    }
    const auto result = runtime.Flush(state);
    runtime.FinishNoOp(state);
    return result;
}

}  // namespace

std::uint32_t CeguiVertexProcessor::ProcessScaled(
    CeguiRendererState& state, const std::int32_t config_width,
    const std::int32_t config_height, ICeguiVertexRuntime& runtime) {
    const auto width_scale = static_cast<float>(config_width) /
                             state.ReadFloat(256u);
    const auto height_scale = static_cast<float>(config_height) /
                              state.ReadFloat(248u);
    runtime.SetRenderStates();
    return ProcessVertices(state, width_scale, height_scale, 0.0f, runtime);
}

std::uint32_t CeguiVertexProcessor::ProcessNative(
    CeguiRendererState& state, ICeguiVertexRuntime& runtime) {
    constexpr std::uint32_t ids[6]{12u, 6u, 8u, 7u, 9u, 2u};
    constexpr std::uint32_t temporary[6]{1u, 0u, 0u, 2u, 1u, 3u};
    std::uint32_t saved[6]{};
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        saved[index] = runtime.GetRenderState(ids[index]);
    }
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        static_cast<void>(runtime.SetRenderState(ids[index], temporary[index]));
    }
    static_cast<void>(ProcessVertices(state, 1.0f, 1.0f, 0.0f, runtime));
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        result = runtime.SetRenderState(ids[index], saved[index]);
    }
    return result;
}

std::uint32_t CeguiVertexProcessor::ProcessRendererScaled(
    CeguiRendererState& state, ICeguiVertexRuntime& runtime) {
    constexpr std::uint32_t ids[6]{12u, 6u, 8u, 7u, 9u, 2u};
    std::uint32_t saved[6]{};
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        saved[index] = runtime.GetRenderState(ids[index]);
    }
    runtime.SetRenderStates();
    static_cast<void>(ProcessVertices(state, state.ReadFloat(272u),
                                      state.ReadFloat(276u), 0.0f,
                                      runtime));
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        result = runtime.SetRenderState(ids[index], saved[index]);
    }
    return result;
}

std::uint32_t CeguiVertexProcessor::ProcessWide(
    CeguiRendererState& state, ICeguiVertexRuntime& runtime) {
    runtime.SetRenderStates();
    return ProcessVertices(state, state.ReadFloat(272u),
                           state.ReadFloat(276u), state.ReadFloat(280u),
                           runtime);
}

}  // namespace pal4
