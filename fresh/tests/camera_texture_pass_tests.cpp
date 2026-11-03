#include "pal4/camera_texture_pass.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICameraTexturePassRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::vector<std::uint32_t> raster_writes;
    std::array<pal4::CameraTextureVertex, 4u> vertices{};
    std::uint32_t raster_result = 0x2222u;
    std::uint32_t begin_result = 1u;
    std::vector<std::uint32_t> begin_results;
    bool drew = false;

    [[nodiscard]] std::uint32_t CreateRaster(
        const std::uint32_t width, const std::uint32_t height,
        const std::uint32_t depth, const std::uint32_t type) override {
        Check(width == 256u && height == 256u && depth == 0u && type == 5u,
              "square type-five raster arguments");
        calls.emplace_back("create");
        return raster_result;
    }
    [[nodiscard]] std::int32_t ReadSignedWord(
        const std::uint32_t address) override {
        Check(address == 0x200cu, "raster width offset");
        calls.emplace_back("size");
        return 256;
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x1084u, "camera reciprocal W source");
        calls.emplace_back("w");
        return 2.0f;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x1060u, "camera raster slot");
        calls.emplace_back("write");
        raster_writes.push_back(value);
    }
    [[nodiscard]] std::uint32_t BeginUpdate(
        const std::uint32_t camera) override {
        Check(camera == 0x1000u, "begin camera");
        calls.emplace_back("begin");
        if (!begin_results.empty()) {
            const auto result = begin_results.front();
            begin_results.erase(begin_results.begin());
            return result;
        }
        return begin_result;
    }
    void EndUpdate(const std::uint32_t camera) override {
        Check(camera == 0x1000u, "end camera");
        calls.emplace_back("end");
    }
    [[nodiscard]] std::uint32_t SetDeviceState(
        const std::uint32_t state, const std::uint32_t value) override {
        calls.emplace_back("state");
        states.emplace_back(state, value);
        return 0xfeedu;
    }
    [[nodiscard]] float DeviceDepth() override {
        calls.emplace_back("depth");
        return 0.25f;
    }
    void DrawTexturedQuad(
        const std::uint32_t primitive,
        const std::array<pal4::CameraTextureVertex, 4u>& data,
        const std::uint32_t count) override {
        Check(primitive == 4u && count == 4u,
              "textured quad primitive and count");
        calls.emplace_back("draw");
        drew = true;
        vertices = data;
    }
};

void CheckVertices(const std::array<pal4::CameraTextureVertex, 4u>& v,
                   const float left, const float top,
                   const float right, const float bottom,
                   const float depth, const float reciprocal_w,
                   const float offset) {
    Check(v[0].x == left && v[0].y == top &&
              v[1].x == left && v[1].y == bottom &&
              v[2].x == right && v[2].y == top &&
              v[3].x == right && v[3].y == bottom,
          "four vertex positions");
    Check(v[0].u == offset && v[0].v == offset &&
              v[1].u == offset && v[1].v == offset + 1.0f &&
              v[2].u == offset + 1.0f && v[2].v == offset &&
              v[3].u == offset + 1.0f && v[3].v == offset + 1.0f,
          "four UV pairs");
    for (const auto& vertex : v) {
        Check(vertex.z == depth && vertex.reciprocal_w == reciprocal_w &&
                  vertex.color == 0xffffffffu,
              "depth, reciprocal W and white color");
    }
}

void TestSquareRasterAndProjectionQuad() {
    Runtime runtime;
    Check(pal4::CameraTexturePass::CreateSquareRaster(256u, runtime) ==
              0x2222u, "raster creation result forwarded");
    runtime.raster_result = 0u;
    Check(pal4::CameraTexturePass::CreateSquareRaster(256u, runtime) == 0u,
          "raster creation failure forwarded");
    Check(runtime.calls == std::vector<std::string>{"create", "create"},
          "one native creation call per wrapper invocation");

    runtime.calls.clear();
    Check(pal4::CameraTexturePass::DrawProjectionQuad(
              1.0f, 2.0f, 31.0f, 42.0f, 0.4f, 0.5f,
              0.125f, runtime) == 1u,
          "projection quad returns one");
    Check(runtime.calls == std::vector<std::string>{"draw"} && runtime.drew,
          "projection quad submits once");
    CheckVertices(runtime.vertices, 1.0f, 2.0f, 31.0f, 42.0f,
                  0.4f, 0.5f, 0.125f);
}

void TestBeginFailure() {
    Runtime runtime;
    runtime.begin_result = 0u;
    Check(pal4::CameraTexturePass::RenderToTexture(
              0x2000u, 0x3000u, 0x1000u, runtime) == 1u,
          "failed begin still returns one");
    Check(runtime.calls == std::vector<std::string>{
              "size", "write", "w", "begin", "write"},
          "begin failure still unbinds raster");
    Check(runtime.raster_writes == std::vector<std::uint32_t>{
              0x2000u, 0u} && !runtime.drew && runtime.states.empty(),
          "failed begin skips device calls");
}

void TestRenderToTexture() {
    Runtime runtime;
    Check(pal4::CameraTexturePass::RenderToTexture(
              0x2000u, 0x3000u, 0x1000u, runtime) == 1u,
          "render returns one regardless of setter results");
    Check(runtime.raster_writes == std::vector<std::uint32_t>{
              0x2000u, 0u}, "temporary camera raster binding");
    Check(runtime.states ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {10u, 2u}, {11u, 1u}, {6u, 0u}, {8u, 0u},
                  {9u, 2u}, {1u, 0x3000u}, {6u, 1u}, {8u, 1u},
                  {10u, 5u}, {11u, 6u}},
          "device state setup, texture binding and restoration");
    Check(runtime.calls == std::vector<std::string>{
              "size", "write", "w", "begin",
              "state", "state", "state", "state", "state", "state",
              "depth", "draw", "state", "state", "state", "state",
              "end", "write"},
          "render and unbind order");
    Check(runtime.drew, "successful begin draws quad");
    CheckVertices(runtime.vertices, 0.0f, 0.0f, 256.0f, 256.0f,
                  0.25f, 0.5f, 0.5f / 256.0f);
}

void TestRepeatedZeroAndGatedPasses() {
    Runtime runtime;
    Check(pal4::CameraTexturePass::RenderToTextureRepeated(
              0x2000u, 0x3000u, 0x1000u, 0u, runtime) == 1u,
          "zero iterations returns one");
    Check(runtime.calls == std::vector<std::string>{
              "size", "w", "write"} &&
              runtime.raster_writes == std::vector<std::uint32_t>{0u},
          "zero iterations still clears camera raster");

    Runtime gated;
    gated.begin_results = {0u, 0u};
    Check(pal4::CameraTexturePass::RenderToTextureRepeated(
              0x2000u, 0x3000u, 0x1000u, 1u, gated) == 1u,
          "two failed begins return one");
    Check(gated.calls == std::vector<std::string>{
              "size", "w", "write", "begin", "write", "begin", "write"} &&
              gated.raster_writes == std::vector<std::uint32_t>{
                  0x3000u, 0x2000u, 0u} && !gated.drew,
          "both begins are independent and raster is cleared");
}

void TestRepeatedDrawing() {
    Runtime runtime;
    Check(pal4::CameraTexturePass::RenderToTextureRepeated(
              0x2000u, 0x3000u, 0x1000u, 2u, runtime) == 1u,
          "two iterations return one");
    Check(runtime.raster_writes == std::vector<std::uint32_t>{
              0x3000u, 0x2000u, 0x3000u, 0x2000u, 0u},
          "alternating raster binding and final clear");
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> one_iteration{
        {10u, 2u}, {11u, 1u}, {6u, 0u}, {8u, 0u},
        {9u, 2u}, {1u, 0x2000u}, {1u, 0x3000u},
        {6u, 1u}, {8u, 1u}, {10u, 5u}, {11u, 6u}};
    Check(runtime.states.size() == one_iteration.size() * 2u,
          "eleven state writes per iteration");
    for (std::size_t i = 0; i < one_iteration.size(); ++i) {
        Check(runtime.states[i] == one_iteration[i] &&
                  runtime.states[i + one_iteration.size()] == one_iteration[i],
              "two-pass state sequence repeats");
    }
    Check(runtime.drew && runtime.calls.back() == "write",
          "repeated draws end with raster clear");
    CheckVertices(runtime.vertices, 0.0f, 0.0f, 256.0f, 256.0f,
                  0.25f, 0.5f, 0.0f);

    Runtime first_failed;
    first_failed.begin_results = {0u, 1u};
    Check(pal4::CameraTexturePass::RenderToTextureRepeated(
              0x2000u, 0x3000u, 0x1000u, 1u, first_failed) == 1u &&
              first_failed.states ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {1u, 0x3000u}, {6u, 1u}, {8u, 1u},
                      {10u, 5u}, {11u, 6u}},
          "second pass runs when first begin fails");

    Runtime second_failed;
    second_failed.begin_results = {1u, 0u};
    Check(pal4::CameraTexturePass::RenderToTextureRepeated(
              0x2000u, 0x3000u, 0x1000u, 1u, second_failed) == 1u &&
              second_failed.states.size() == 6u &&
              second_failed.states.back() ==
                  std::pair<std::uint32_t, std::uint32_t>{1u, 0x2000u},
          "first pass runs when second begin fails");
}

}  // namespace

int main() {
    TestSquareRasterAndProjectionQuad();
    TestBeginFailure();
    TestRenderToTexture();
    TestRepeatedZeroAndGatedPasses();
    TestRepeatedDrawing();
}
