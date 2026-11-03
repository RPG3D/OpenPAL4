#include "pal4/pal_rectangle_drawer.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class RenderRuntime final : public pal4::IPalRectangleRenderRuntime {
public:
    float CameraDepth(std::uint32_t context) override {
        ++camera_calls;
        Check(context == 0x1000u, "camera depth receives context address");
        return 2.0f;
    }
    float ActiveDepth() override {
        return 0.25f + static_cast<float>(depth_calls++);
    }
    std::uint32_t TextureRaster(std::uint32_t texture) override {
        events.push_back("texture");
        Check(texture == 0x2000u, "texture pointer is forwarded");
        return 0x3000u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        states.push_back({state, value});
        events.push_back("state:" + std::to_string(state));
    }
    void DrawPrimitive(
        std::uint32_t primitive,
        const std::array<pal4::CeguiRenderVertex, 4>& input,
        std::uint32_t count) override {
        ++draw_calls;
        events.push_back("draw");
        Check(primitive == 4u && count == 4u,
              "RenderWare draws four vertices with primitive type 4");
        vertices = input;
    }
    std::array<pal4::CeguiRenderVertex, 4> vertices{};
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::vector<std::string> events;
    int camera_calls{};
    int depth_calls{};
    int draw_calls{};
};

void MissingContextOrTextureSkipsRenderWare() {
    RenderRuntime runtime;
    const pal4::PalRectangleColor color{0x33u, 0x22u, 0x11u, 0x44u};
    const std::array<std::uint32_t, 8> uv{};
    Check(pal4::PalRectangleDrawer::DrawFilled(
              0u, 10, 20, 30, 40, color, uv, runtime) == 0u &&
              pal4::PalRectangleDrawer::DrawTextured(
                  0u, 10, 20, 30, 40, 0x2000u,
                  color, runtime) == 0u &&
              pal4::PalRectangleDrawer::DrawTexturedWithAlpha(
                  0x1000u, 10, 20, 30, 40, 0u,
                  color, runtime) == 0u,
          "missing context or texture returns zero");
    Check(runtime.camera_calls == 0 && runtime.draw_calls == 0 &&
              runtime.events.empty(),
          "null inputs avoid all runtime calls");
}

void FilledRectanglePreservesUnknownUVAndStateOrder() {
    RenderRuntime runtime;
    const pal4::PalRectangleColor color{0x33u, 0x22u, 0x11u, 0x44u};
    const std::array<std::uint32_t, 8> uv{
        0x3f000000u, 0x3f800000u, 0x40000000u, 0x40400000u,
        0x40800000u, 0x40a00000u, 0x40c00000u, 0x40e00000u};
    Check(pal4::PalRectangleDrawer::DrawFilled(
              0x1000u, 10, 20, 30, 40, color, uv, runtime) == 1u,
          "filled rectangle returns one on success");
    Check(runtime.camera_calls == 1 && runtime.depth_calls == 4 &&
              runtime.draw_calls == 1,
          "camera reciprocal is read once and active depth four times");
    Check(runtime.vertices[0].x == 10.0f &&
              runtime.vertices[0].y == 60.0f &&
              runtime.vertices[1].x == 40.0f &&
              runtime.vertices[1].y == 60.0f &&
              runtime.vertices[2].x == 10.0f &&
              runtime.vertices[2].y == 20.0f &&
              runtime.vertices[3].x == 40.0f &&
              runtime.vertices[3].y == 20.0f,
          "vertex order is bottom-left, bottom-right, top-left, top-right");
    for (std::size_t index = 0u; index < 4u; ++index) {
        Check(runtime.vertices[index].reciprocal_camera_depth == 0.5f &&
                  runtime.vertices[index].depth ==
                      0.25f + static_cast<float>(index) &&
                  runtime.vertices[index].colour == 0x44332211u &&
                  std::bit_cast<std::uint32_t>(runtime.vertices[index].u) ==
                      uv[2u * index] &&
                  std::bit_cast<std::uint32_t>(runtime.vertices[index].v) ==
                      uv[2u * index + 1u],
              "filled vertex preserves color, depth and incoming UV bits");
    }
    Check(runtime.states ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {12u, 1u}, {6u, 0u}, {8u, 0u}, {7u, 2u},
                  {9u, 2u}, {2u, 3u}, {1u, 0u}}) &&
              runtime.events.back() == "draw",
          "filled path applies seven states before drawing");
}

void TexturedVariantsSetUVAndAlphaStates() {
    RenderRuntime plain;
    const pal4::PalRectangleColor color{1u, 2u, 3u, 4u};
    Check(pal4::PalRectangleDrawer::DrawTextured(
              0x1000u, 3, 4, 5, 6, 0x2000u,
              color, plain) == 1u,
          "textured rectangle returns one");
    Check(plain.vertices[0].u == 0.0f && plain.vertices[0].v == 1.0f &&
              plain.vertices[1].u == 1.0f && plain.vertices[1].v == 1.0f &&
              plain.vertices[2].u == 0.0f && plain.vertices[2].v == 0.0f &&
              plain.vertices[3].u == 1.0f && plain.vertices[3].v == 0.0f,
          "textured UVs span the full image");
    Check(plain.states ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {6u, 0u}, {8u, 0u}, {7u, 2u}, {9u, 2u},
                  {2u, 3u}, {1u, 0x3000u}}),
          "plain texture omits alpha setup and binds raster last");
    Check(plain.events.at(5) == "texture" &&
              plain.events.at(6) == "state:1" &&
              plain.events.at(7) == "draw",
          "raster lookup follows common states and precedes binding");

    RenderRuntime alpha;
    Check(pal4::PalRectangleDrawer::DrawTexturedWithAlpha(
              0x1000u, 3, 4, 5, 6, 0x2000u,
              color, alpha) == 1u &&
              alpha.states ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {12u, 1u}, {10u, 5u}, {11u, 6u},
                      {6u, 0u}, {8u, 0u}, {7u, 2u},
                      {9u, 2u}, {2u, 3u}, {1u, 0x3000u}}),
          "alpha texture configures source/destination blend first");
}

void EmptyRuntimeStubLinksAndDraws() {
    pal4::PalRectangleRenderRuntimeStub runtime;
    Check(pal4::PalRectangleDrawer::DrawFilled(
              1u, 0, 0, 1, 1, {}, {}, runtime) == 1u &&
              pal4::PalRectangleDrawer::DrawTextured(
                  1u, 0, 0, 1, 1, 2u, {}, runtime) == 1u,
          "linkable RenderWare stub accepts both draw paths");
}

}  // namespace

int main() {
    MissingContextOrTextureSkipsRenderWare();
    FilledRectanglePreservesUnknownUVAndStateOrder();
    TexturedVariantsSetUVAndAlphaStates();
    EmptyRuntimeStubLinksAndDraws();
    std::cout << "4 PAL rectangle drawing test cases passed\n";
}
