#include "pal4/cegui_immediate_rectangle.h"

#include <bit>

namespace pal4 {

std::int32_t CeguiImmediateRectangleStub::Convert(const double value) {
    CeguiHostFloatToInt host;
    return host.Convert(value);
}
float CeguiImmediateRectangleStub::ActiveCameraDepth() { return 1.0f; }
float CeguiImmediateRectangleStub::RenderDepth() { return 0.0f; }
std::uint32_t CeguiImmediateRectangleStub::GetRenderState(
    std::uint32_t /*id*/) { return 0u; }
std::uint32_t CeguiImmediateRectangleStub::SetRenderState(
    std::uint32_t /*id*/, std::uint32_t /*value*/) { return 0u; }
void CeguiImmediateRectangleStub::BindTexture(
    std::uint32_t /*texture_handle*/) {}
std::uint32_t CeguiImmediateRectangleStub::DrawTriangleStrip(
    const std::array<CeguiRenderVertex, 4>& /*vertices*/) { return 0u; }

void CeguiImmediateRectangle::EmptyRendererSlot() noexcept {}

namespace {

float FloatWord(const std::uint32_t word) {
    return std::bit_cast<float>(word);
}

std::uint32_t PackedColour(const CeguiColourWords& colour,
                           ICeguiImmediateRectangleRuntime& runtime) {
    CeguiPackedColor packed;
    static_cast<void>(packed.SetAllBytesFF());
    static_cast<void>(packed.SetFromFloats(
        colour[0], colour[1], colour[2], colour[3], runtime));
    const auto& bytes = packed.Bytes();
    return (static_cast<std::uint32_t>(bytes[3]) << 24u) |
           (static_cast<std::uint32_t>(bytes[0]) << 16u) |
           (static_cast<std::uint32_t>(bytes[1]) << 8u) |
           static_cast<std::uint32_t>(bytes[2]);
}

std::array<CeguiRenderVertex, 4> BuildVertices(
    const CeguiRectWords& rectangle,
    const CeguiRectWords& uv,
    const std::array<CeguiColourWords, 4>& colours,
    const float width_scale, const float height_scale,
    const float horizontal_bias, const float pixel_bias,
    ICeguiImmediateRectangleRuntime& runtime) {
    std::array<std::uint32_t, 4> packed{};
    for (std::size_t index = 0u; index < packed.size(); ++index) {
        packed[index] = PackedColour(colours[index], runtime);
    }

    const auto reciprocal_depth = 1.0f / runtime.ActiveCameraDepth();
    const auto depth = runtime.RenderDepth();
    const auto left = FloatWord(rectangle[2]) * width_scale +
                      horizontal_bias - pixel_bias;
    const auto top = FloatWord(rectangle[0]) * height_scale - pixel_bias;
    const auto right = FloatWord(rectangle[3]) * width_scale +
                       horizontal_bias - pixel_bias;
    const auto bottom = FloatWord(rectangle[1]) * height_scale - pixel_bias;
    const auto v0 = FloatWord(uv[0]);
    const auto v1 = FloatWord(uv[1]);
    const auto u0 = FloatWord(uv[2]);
    const auto u1 = FloatWord(uv[3]);
    return {{
        {left, bottom, depth, reciprocal_depth, packed[2], u0, v1},
        {right, bottom, depth, reciprocal_depth, packed[3], u1, v1},
        {left, top, depth, reciprocal_depth, packed[0], u0, v0},
        {right, top, depth, reciprocal_depth, packed[1], u1, v0},
    }};
}

std::uint32_t DrawCore(
    const CeguiRectWords& rectangle, const std::uint32_t texture_handle,
    const CeguiRectWords& uv,
    const std::array<CeguiColourWords, 4>& colours,
    const float width_scale, const float height_scale,
    const std::uint32_t state_nine,
    ICeguiImmediateRectangleRuntime& runtime) {
    const auto vertices = BuildVertices(rectangle, uv, colours,
                                        width_scale, height_scale,
                                        0.0f, 0.5f, runtime);
    constexpr std::array<std::uint32_t, 6> ids{12u, 6u, 8u, 7u, 9u, 2u};
    const std::array<std::uint32_t, 6> temporary{
        1u, 0u, 0u, 2u, state_nine, 3u};
    std::array<std::uint32_t, 6> saved{};
    for (std::size_t index = 0u; index < ids.size(); ++index) {
        saved[index] = runtime.GetRenderState(ids[index]);
    }
    for (std::size_t index = 0u; index < ids.size(); ++index) {
        static_cast<void>(runtime.SetRenderState(ids[index], temporary[index]));
    }
    runtime.BindTexture(texture_handle);
    static_cast<void>(runtime.DrawTriangleStrip(vertices));
    std::uint32_t result = 0u;
    for (std::size_t index = 0u; index < ids.size(); ++index) {
        result = runtime.SetRenderState(ids[index], saved[index]);
    }
    return result;
}

}  // namespace

std::uint32_t CeguiImmediateRectangle::Draw(
    const CeguiRectWords& rectangle, const std::uint32_t texture_handle,
    const CeguiRectWords& uv,
    const std::array<CeguiColourWords, 4>& colours,
    ICeguiImmediateRectangleRuntime& runtime) {
    return DrawCore(rectangle, texture_handle, uv, colours,
                    1.0f, 1.0f, 1u, runtime);
}

std::uint32_t CeguiImmediateRectangle::DrawScaled(
    const CeguiRendererState& state,
    const CeguiRectWords& rectangle, const std::uint32_t texture_handle,
    const CeguiRectWords& uv,
    const std::array<CeguiColourWords, 4>& colours,
    ICeguiImmediateRectangleRuntime& runtime) {
    return DrawCore(rectangle, texture_handle, uv, colours,
                    state.ReadFloat(272u), state.ReadFloat(276u),
                    2u, runtime);
}

std::uint32_t CeguiImmediateRectangle::DrawOffset(
    const CeguiRendererState& state,
    const CeguiRectWords& rectangle, const std::uint32_t texture_handle,
    const CeguiRectWords& uv,
    const std::array<CeguiColourWords, 4>& colours,
    ICeguiImmediateRectangleRuntime& runtime) {
    const auto vertices = BuildVertices(
        rectangle, uv, colours, state.ReadFloat(272u),
        state.ReadFloat(276u), state.ReadFloat(280u), 0.0f, runtime);
    constexpr std::array<std::uint32_t, 6> ids{12u, 6u, 8u, 7u, 9u, 2u};
    constexpr std::array<std::uint32_t, 6> values{1u, 0u, 0u, 2u, 2u, 3u};
    for (std::size_t index = 0u; index < ids.size(); ++index) {
        static_cast<void>(runtime.SetRenderState(ids[index], values[index]));
    }
    runtime.BindTexture(texture_handle);
    return runtime.DrawTriangleStrip(vertices);
}

}  // namespace pal4
