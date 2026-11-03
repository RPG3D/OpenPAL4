#include "pal4/dynamic_strip_object_render.h"

#include <bit>
#include <algorithm>
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

void Byte(pal4::DynamicStripObjectState& state,
          std::uint32_t offset, std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}

void Float(pal4::DynamicStripObjectState& state,
           std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IDynamicStripVisualRenderRuntime {
    std::vector<pal4::DynamicStripSegment> segments;
    std::vector<std::string> calls;
    std::vector<pal4::DynamicStripIm3DVertex> vertices;
    std::vector<std::uint16_t> indices;

    std::vector<pal4::DynamicStripSegment> Segments(
        std::uint32_t sentinel) override {
        Check(sentinel == 0x700u, "list sentinel");
        calls.push_back("segments");
        return segments;
    }
    std::uint32_t TextureRaster(std::uint32_t texture) override {
        Check(texture == 0x600u, "texture handle");
        calls.push_back("texture");
        return 0x601u;
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
    void Transform(const std::vector<pal4::DynamicStripIm3DVertex>& value,
                   std::uint32_t flags) override {
        Check(flags == 29u, "Im3D transform flags");
        calls.push_back("transform");
        vertices = value;
    }
    void RenderIndexed(std::uint32_t primitive,
                       const std::vector<std::uint16_t>& value) override {
        Check(primitive == 3u, "triangle-list primitive");
        calls.push_back("indexed");
        indices = value;
    }
    void End() override { calls.push_back("end"); }
};

pal4::DynamicStripObjectState State() {
    pal4::DynamicStripObjectState state;
    Byte(state, 0u, 1u);
    Byte(state, 1u, 1u);
    Byte(state, 52u, 0x12u);
    Byte(state, 53u, 0x34u);
    Byte(state, 54u, 0x56u);
    Byte(state, 55u, 255u);
    Float(state, 60u, 2.0f);
    Float(state, 68u, 0.25f);
    Float(state, 72u, 0.75f);
    state.words[16u] = 0x600u;
    state.words[28u] = 0x700u;
    state.words[29u] = 3u;
    return state;
}

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.segments = {
        {{1, 2, 3}, {4, 5, 6}, 2.0f},
        {{7, 8, 9}, {10, 11, 12}, 1.0f},
        {{13, 14, 15}, {16, 17, 18}, 0.0f},
    };
    return runtime;
}

void Gates() {
    auto state = State();
    auto runtime = MakeRuntime();
    Byte(state, 0u, 0u);
    pal4::DynamicStripObjectRenderer::RenderVisual(state, runtime);
    Check(runtime.calls.empty(), "inactive gate");
    Byte(state, 0u, 1u);
    state.words[29u] = 1u;
    pal4::DynamicStripObjectRenderer::RenderVisual(state, runtime);
    Check(runtime.calls.empty(), "segment count gate");
}

void GeometryAndStates() {
    auto state = State();
    auto runtime = MakeRuntime();
    pal4::DynamicStripObjectRenderer::RenderVisual(state, runtime);
    Check(runtime.vertices.size() == 8u && runtime.indices.size() == 12u,
          "two quads and twelve indices");
    Check(runtime.indices == std::vector<std::uint16_t>{
              0, 2, 1, 1, 2, 3, 4, 6, 5, 5, 6, 7}, "triangle winding");
    const auto& v = runtime.vertices;
    Check(v[0].x == 1.0f && v[1].x == 4.0f && v[2].x == 7.0f &&
              v[3].x == 10.0f && v[4].x == 7.0f && v[6].x == 13.0f,
          "segment positions and shared edge");
    Check(v[0].color == 0xff123456u && v[2].color == 0x80123456u &&
              v[6].color == 0x00123456u, "authored alpha times remaining/life");
    Check(v[0].u == 0.25f && v[2].u == 0.75f && v[6].u == 1.0f &&
              v[0].v == 0.75f && v[1].v == 1.0f,
          "UV interpolation and upper clamp");
    const std::vector<std::string> expected{
        "segments", "texture", "set-dev:1:1537", "get-rw:27",
        "get-rw:19", "get-rw:20", "set-rw:27:1", "set-rw:19:5",
        "set-rw:20:2", "get-rw:7", "get-rw:14", "set-rw:7:1",
        "set-rw:14:0", "set-dev:9:2", "get-dev:20", "set-dev:20:1",
        "transform", "indexed", "end", "set-dev:1:0", "set-dev:20:220",
        "set-rw:7:107", "set-rw:14:114", "set-rw:27:127",
        "set-rw:19:119", "set-rw:20:120"};
    Check(runtime.calls == expected, "native state transaction order");
}

void EndpointModeAndRounding() {
    auto state = State();
    auto runtime = MakeRuntime();
    Byte(state, 4u, 1u);
    Byte(state, 1u, 0u);
    Byte(state, 55u, 5u);
    runtime.segments.front().remaining = 1.0f;
    runtime.segments.back().remaining = 2.0f;
    pal4::DynamicStripObjectRenderer::RenderVisual(state, runtime);
    Check(runtime.vertices.size() == 4u && runtime.indices.size() == 6u,
          "endpoint mode creates one quad");
    Check(runtime.vertices[0].color == 0x02123456u &&
              runtime.vertices[2].color == 0x05123456u,
          "CRT_Ftol nearest-even alpha");
    Check(runtime.vertices[2].x == 13.0f && runtime.vertices[2].u == 1.0f,
          "endpoint mode copies list tail");
    Check(runtime.calls[8] == "set-rw:20:6", "opaque blend mode");
}

}  // namespace

int main() {
    Gates();
    GeometryAndStates();
    EndpointModeAndRounding();
    auto state = State();
    struct Guarded {
        std::uint32_t before = 0x12345678u;
        std::array<std::uint32_t, 34u> words{};
        std::uint32_t after = 0x87654321u;
    } block;
    std::copy_n(state.words.begin(), block.words.size(), block.words.begin());
    Runtime original, embedded;
    original.segments = embedded.segments = {
        {{1.0f,2.0f,3.0f},{4.0f,5.0f,6.0f},1.0f},
        {{7.0f,8.0f,9.0f},{10.0f,11.0f,12.0f},0.5f}};
    state.words[29u] = block.words[29u] = 2u;
    pal4::DynamicStripObjectRenderer::RenderVisual(state, original);
    pal4::DynamicStripObjectRenderer::RenderVisual(
        pal4::StripVisualBlockConstView{block.words}, embedded);
    Check(original.calls == embedded.calls && !embedded.vertices.empty() &&
              original.vertices.size() == embedded.vertices.size() &&
              original.indices == embedded.indices &&
              original.vertices[0u].color == embedded.vertices[0u].color &&
              original.vertices[0u].x == embedded.vertices[0u].x,
          "136-byte embedded block shares geometry and render-state behavior");
    Check(block.before == 0x12345678u && block.after == 0x87654321u,
          "embedded drawing preserves adjacent blocks");
    pal4::DynamicStripVisualRenderRuntimeStub stub;
    pal4::DynamicStripObjectRenderer::RenderVisual(state, stub);
}
