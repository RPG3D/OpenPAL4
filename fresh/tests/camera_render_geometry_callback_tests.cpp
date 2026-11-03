#include "pal4/camera_render_geometry_callback.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Triple = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;
using TextureBinding = std::pair<std::uint32_t, std::uint32_t>;

struct Runtime final : pal4::ICameraRenderGeometryCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<Triple> stages;
    std::vector<TextureBinding> textures;
    std::vector<std::uint32_t> alpha_changes;
    std::vector<std::uint32_t> colors;
    std::uint32_t render_state{0u};
    std::uint32_t alpha_state{0u};

    Runtime() {
        words[0x101cu] = 0u;
        words[0x1020u] = 0u;
        words[0x1024u] = 4u;
        words[0x1048u] = 0xa0u;
        words[0x104cu] = 0xb0u;
        AddMesh(0u, 0x2000u, 0u, 0xffu);
        AddMesh(1u, 0x2100u, 0u, 0x80u);
    }
    void AddMesh(const std::uint32_t index,
                 const std::uint32_t material,
                 const std::uint32_t mesh_flag,
                 const std::uint8_t alpha) {
        const auto mesh = 0x1064u + index * 36u;
        words[mesh - 4u] = material;
        words[mesh] = mesh_flag;
        words[mesh + 4u] = 0x7000u + index;
        words[mesh + 8u] = 5u + index;
        words[mesh + 12u] = 10u + index;
        words[mesh + 16u] = 1u + index;
        words[mesh + 20u] = 30u + index;
        words[material] = 0x3300u + index;
        bytes[material + 4u] = 1u;
        bytes[material + 5u] = 2u;
        bytes[material + 6u] = 3u;
        bytes[material + 7u] = alpha;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(words.contains(address), "render-data word offset");
        return words[address];
    }
    [[nodiscard]] std::uint8_t ReadByte(
        const std::uint32_t address) override {
        Check(bytes.contains(address), "material color byte offset");
        return bytes[address];
    }
    void SetPixelShader(const std::uint32_t shader) override {
        Check(shader == 0u, "pixel shader cleared");
        calls.push_back("pixel");
    }
    void EnableClipping(const std::uint32_t clipping,
                        const std::uint8_t flags) override {
        Check(clipping == 0x55u && flags == 0xf2u,
              "clipping argument and low flag byte");
        calls.push_back("clip");
    }
    void SetIndices(const std::uint32_t indices) override {
        Check(indices == 0x6000u, "index buffer value");
        calls.push_back("indices");
    }
    void SetStreams(const std::uint32_t streams,
                    const std::uint32_t value) override {
        Check(streams == 0x1028u && value == 0xa0u,
              "stream field offsets");
        calls.push_back("streams");
    }
    void SetVertexDeclaration(
        const std::uint32_t declaration) override {
        Check(declaration == 0xb0u, "vertex declaration offset");
        calls.push_back("declaration");
    }
    [[nodiscard]] std::uint32_t GetRenderState(
        const std::uint32_t state) override {
        Check(state == 137u, "render state query index");
        calls.push_back("get-render-state");
        return render_state;
    }
    [[nodiscard]] std::uint32_t GetDeviceState(
        const std::uint32_t state) override {
        Check(state == 12u, "device alpha state query");
        calls.push_back("get-device-state");
        return alpha_state;
    }
    void SetDeviceState(const std::uint32_t state,
                        const std::uint32_t value) override {
        Check(state == 12u && value <= 1u, "device alpha state write");
        calls.push_back("set-device-state");
        alpha_changes.push_back(value);
    }
    void SetTexture(const std::uint32_t texture,
                         const std::uint32_t stage) override {
        textures.emplace_back(texture, stage);
    }
    void SetTextureStageState(
        const std::uint32_t stage,
        const std::uint32_t state,
        const std::uint32_t value) override {
        stages.emplace_back(stage, state, value);
    }
    void SetSurfaceProperties(const std::uint32_t surface,
                              const std::uint32_t color,
                              const std::uint32_t flags) override {
        Check((surface == 0x200cu && color == 0x2004u) ||
                  (surface == 0x210cu && color == 0x2104u),
              "surface property material offsets");
        Check(flags == 0u, "surface property render flags");
        calls.push_back("surface");
    }
    void SetRenderState(const std::uint32_t state,
                        const std::uint32_t value) override {
        Check(state == 60u, "packed-color render state index");
        calls.push_back("color");
        colors.push_back(value);
    }
    void SetVertexShader(const std::uint32_t shader) override {
        Check(shader == 0x7000u || shader == 0x7001u,
              "mesh vertex shader offset");
        calls.push_back("shader");
    }
    void DrawIndexed(const std::uint32_t primitive,
                     const std::uint32_t first,
                     const std::uint32_t base,
                     const std::uint32_t count,
                     const std::uint32_t start,
                     const std::uint32_t index_count) override {
        Check(primitive == 4u && first == 5u && base == 0u &&
                  count == 10u && start == 1u && index_count == 30u,
              "indexed draw arguments");
        calls.push_back("draw-indexed");
    }
    void DrawPrimitive(const std::uint32_t primitive,
                       const std::uint32_t first,
                       const std::uint32_t count) override {
        Check(primitive == 4u && (first == 5u || first == 6u) &&
                  count == first + 5u, "non-indexed draw arguments");
        calls.push_back("draw-primitive");
    }
};

void Render(Runtime& runtime, const std::uint32_t flags,
                     const float strength) {
    const pal4::CameraProjectionGlobals projection{
        0x3000u, std::bit_cast<std::uint32_t>(strength)};
    pal4::CameraRenderGeometryCallback::Render(
        0x1000u, 0x55u, 0xf2u,
        flags, projection, runtime);
}

void TestPlainSurfaceAndAlphaTransitions() {
    Runtime runtime;
    runtime.words[0x101cu] = 2u;
    runtime.render_state = 7u;
    runtime.alpha_state = 1u;
    Render(runtime, 0u, 1.0f);
    Check(runtime.calls == std::vector<std::string>{
              "pixel", "clip", "streams", "declaration",
              "get-render-state", "get-device-state", "set-device-state",
              "surface", "shader", "draw-primitive", "set-device-state",
              "surface", "shader", "draw-primitive"},
          "plain mesh setup, draw and alpha transition order");
    Check(runtime.alpha_changes == std::vector<std::uint32_t>{0u, 1u},
          "alpha is disabled for opaque mesh and enabled for translucent mesh");
    Check(runtime.colors.empty(), "surface properties supersede packed color");
    Check(runtime.textures == std::vector<TextureBinding>{
              {0u, 0u},
              {0x3000u, 1u},
              {0u, 1u}},
          "plain texture stage routing and cleanup");
    Check(runtime.stages == std::vector<Triple>{
              {0u, 1u, 3u}, {0u, 3u, 0u}, {0u, 4u, 3u}, {0u, 6u, 0u},
              {1u, 1u, 4u}, {1u, 2u, 2u}, {1u, 3u, 1u}, {1u, 4u, 3u},
              {1u, 6u, 1u}, {1u, 24u, 2u}, {1u, 11u, 0x20000u},
              {1u, 1u, 1u}, {1u, 4u, 1u}, {1u, 24u, 0u}, {1u, 11u, 1u}},
          "plain stage-zero and stage-one state sequence");
}

void TestTexturedIndexedAndThirdStage() {
    Runtime runtime;
    runtime.words[0x101cu] = 1u;
    runtime.words[0x1020u] = 0x6000u;
    runtime.words[0x1064u] = 1u;
    Render(runtime, 0xc4u, -1.0f);
    Check(runtime.calls == std::vector<std::string>{
              "pixel", "clip", "indices", "streams", "declaration",
              "get-render-state", "get-device-state", "set-device-state",
              "color", "shader", "draw-indexed"},
          "textured indexed draw and packed-color order");
    Check(runtime.colors == std::vector<std::uint32_t>{0xff010203u} &&
              runtime.alpha_changes == std::vector<std::uint32_t>{1u},
          "opaque material with mesh flag enables alpha and packs ARGB");
    Check(runtime.textures == std::vector<TextureBinding>{
              {0x3000u, 1u},
              {0x3300u, 0u},
              {0u, 1u}},
          "textured path binds projection, material and cleanup stages");
    Check(runtime.stages == std::vector<Triple>{
              {0u, 1u, 4u}, {0u, 2u, 2u}, {0u, 3u, 0u},
              {0u, 4u, 4u}, {0u, 5u, 2u}, {0u, 6u, 0u},
              {1u, 1u, 4u}, {1u, 2u, 18u}, {1u, 3u, 1u},
              {1u, 4u, 3u}, {1u, 6u, 1u}, {1u, 24u, 2u},
              {1u, 11u, 0x20000u},
              {2u, 1u, 4u}, {2u, 2u, 3u}, {2u, 3u, 1u},
              {2u, 4u, 4u}, {2u, 5u, 3u}, {2u, 6u, 1u},
              {1u, 1u, 1u}, {1u, 4u, 1u}, {1u, 24u, 0u},
              {1u, 11u, 1u}, {2u, 1u, 1u}, {2u, 4u, 1u}},
          "textured stage-zero, signed stage-one and third-stage restoration");
}

void TestNoMeshStillRestoresStages() {
    Runtime runtime;
    Render(runtime, 0x40u, 0.0f);
    Check(runtime.calls == std::vector<std::string>{
                  "pixel", "clip", "streams", "declaration",
                  "get-render-state", "get-device-state"} &&
              runtime.stages.back() == Triple{2u, 4u, 1u} &&
              runtime.stages[5] == Triple{1u, 2u, 18u},
          "empty mesh array still configures and restores third stage");
}

}  // namespace

int main() {
    TestPlainSurfaceAndAlphaTransitions();
    TestTexturedIndexedAndThirdStage();
    TestNoMeshStillRestoresStages();
}
