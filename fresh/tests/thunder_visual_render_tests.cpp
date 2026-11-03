#include "pal4/thunder_renderer.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void Byte(pal4::ThunderObjectState& state,
          std::uint32_t offset, std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

struct Runtime final : pal4::IThunderVisualRenderRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::ThunderVisualSegment> segments{
        {{1, 2, 3}, {4, 5, 6}},
        {{7, 8, 9}, {10, 11, 12}},
        {{13, 14, 15}, {16, 17, 18}}};
    std::vector<std::array<pal4::ThunderIm3DVertex, 4u>> quads;
    void UpdateTransformProperties(pal4::ThunderObjectState&) override {
        calls.push_back("update-transform");
    }
    void UpdateVisual(pal4::ThunderObjectState&) override {
        calls.push_back("update-visual");
    }
    void UpdateCollection(pal4::ThunderObjectState&) override {
        calls.push_back("update-collection");
    }
    std::vector<pal4::ThunderVisualSegment> ReadSegments(
        std::uint32_t begin, std::uint32_t end) override {
        Check(begin == 0x1000u && end == 0x1048u,
              "native segment buffer range");
        calls.push_back("segments");
        return segments;
    }
    std::uint32_t TextureRaster(std::uint32_t texture) override {
        Check(texture == 0x800u, "bound texture handle");
        calls.push_back("texture");
        return 0x801u;
    }
    std::uint32_t GetRenderState(std::uint32_t state) override {
        calls.push_back("get-rw:" + std::to_string(state));
        return state + 100u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.push_back("set-rw:" + std::to_string(state) + ":" +
                        std::to_string(value));
    }
    std::uint32_t GetDeviceState(std::uint32_t state) override {
        calls.push_back("get-dev:" + std::to_string(state));
        return state + 200u;
    }
    void SetDeviceState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.push_back("set-dev:" + std::to_string(state) + ":" +
                        std::to_string(value));
    }
    void TransformQuad(
        const std::array<pal4::ThunderIm3DVertex, 4u>& quad,
        std::uint32_t flags) override {
        Check(flags == 29u, "Im3D transform flags");
        calls.push_back("transform");
        quads.push_back(quad);
    }
    void RenderIndexed(
        std::uint32_t primitive,
        const std::array<std::uint16_t, 4u>& indices) override {
        Check(primitive == 4u &&
                  indices == std::array<std::uint16_t, 4u>{0u, 1u, 2u, 3u},
              "triangle fan primitive and indices");
        calls.push_back("indexed");
    }
    void End() override { calls.push_back("end"); }
};

pal4::ThunderObjectState State() {
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    Byte(state, 60u, 1u);
    Byte(state, 61u, 1u);
    Byte(state, 108u, 0x12u);
    Byte(state, 109u, 0x34u);
    Byte(state, 110u, 0x56u);
    Byte(state, 111u, 0x78u);
    state.words[29u] = 0x800u;
    state.words[47u] = 0x1000u;
    state.words[48u] = 0x1048u;
    state.words[51u] = 0x2000u;
    state.words[52u] = 0x2090u;
    return state;
}

void GatesAndThreshold() {
    auto state = State();
    Runtime runtime;
    Byte(state, 60u, 0u);
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.calls.empty(), "inactive render gate");

    Byte(state, 60u, 1u);
    state.words[47u] = 0u;
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "update-transform", "update-visual", "update-collection"},
          "near-zero timer updates before missing segment gate");
    runtime.calls.clear();
    state.words[47u] = 0x1000u;
    state.words[48u] = 0x1018u;
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.calls.size() == 3u, "single segment does not render");

    constexpr float epsilon = std::bit_cast<float>(0x38d1b717u);
    state.words[36u] = std::bit_cast<std::uint32_t>(epsilon);
    runtime.calls.clear();
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.calls.empty(), "positive epsilon boundary skips updates");
    state.words[36u] = std::bit_cast<std::uint32_t>(-epsilon);
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.calls.empty(), "negative epsilon boundary skips updates");
}

void QuadsAndStates() {
    auto state = State();
    Runtime runtime;
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.quads.size() == 2u,
          "one fan per adjacent 24-byte segment pair");
    Check(runtime.quads[0][0].x == 1.0f &&
              runtime.quads[0][1].x == 4.0f &&
              runtime.quads[0][2].x == 7.0f &&
              runtime.quads[0][3].x == 10.0f &&
              runtime.quads[1][2].x == 13.0f,
          "fan vertices use both segment endpoints");
    Check(runtime.quads[0][0].color == 0x78123456u &&
              runtime.quads[1][3].color == 0x78123456u &&
              runtime.quads[0][0].u == 0.0f &&
              runtime.quads[0][1].v == 1.0f &&
              runtime.quads[0][2].u == 1.0f &&
              runtime.quads[0][3].v == 1.0f,
          "ARGB color and fixed quad UVs");
    const std::vector<std::string> expected{
        "update-transform", "update-visual", "update-collection",
        "segments", "texture", "set-dev:1:2049", "get-rw:27",
        "get-rw:19", "get-rw:20", "set-rw:27:1", "set-rw:19:5",
        "set-rw:20:2", "get-rw:7", "get-rw:14", "set-rw:7:1",
        "set-rw:14:0", "set-dev:9:2", "get-dev:20", "set-dev:20:1",
        "transform", "indexed", "end", "transform", "indexed", "end",
        "set-dev:1:0", "set-dev:20:220", "set-rw:7:107",
        "set-rw:14:114", "set-rw:27:127", "set-rw:19:119",
        "set-rw:20:120"};
    Check(runtime.calls == expected, "native update and render-state order");
    Check(state.words[52u] == state.words[51u],
          "reusable vertex buffer end reset to begin");
    Check(state.words[0u] == 0xdeadbeefu,
          "outer Thunder object header is untouched");

    state = State();
    Byte(state, 61u, 0u);
    runtime = Runtime{};
    pal4::ThunderRenderer::RenderVisual(state, runtime);
    Check(runtime.calls[11] == "set-rw:20:6", "nonblend state branch");
}

}  // namespace

int main() {
    GatesAndThreshold();
    QuadsAndStates();
    auto state = State();
    pal4::ThunderVisualRenderRuntimeStub stub;
    pal4::ThunderRenderer::RenderVisual(state, stub);
}
