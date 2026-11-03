#include "pal4/camera_fog_overlay.h"

#include <cmath>
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

struct Runtime final : pal4::ICameraFogRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::vector<std::uint32_t> raster_writes;
    std::array<pal4::CameraScreenVertex, 4u> vertices{};
    std::uint32_t bound_raster = 0u;
    std::uint32_t begin_result = 1u;
    std::uint32_t depth_reads = 0u;
    bool drew = false;
    double ftol_argument = 0.0;

    [[nodiscard]] std::int32_t RoundToInt(const float value) override {
        calls.emplace_back("round");
        return static_cast<std::int32_t>(std::nearbyint(value));
    }
    [[nodiscard]] std::int32_t Ftol(const double value) override {
        calls.emplace_back("ftol");
        ftol_argument = value;
        return static_cast<std::int32_t>(value);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x1060u, "camera raster slot");
        calls.emplace_back("write");
        raster_writes.push_back(value);
        bound_raster = value;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(address == 0x1060u, "bound raster read offset");
        calls.emplace_back("raster");
        return bound_raster;
    }
    [[nodiscard]] std::int32_t ReadSignedWord(
        const std::uint32_t address) override {
        Check(address == 0x200cu || address == 0x2010u,
              "raster dimension offsets");
        calls.emplace_back(address == 0x200cu ? "width" : "height");
        return address == 0x200cu ? 640 : 480;
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x1080u, "camera reciprocal W offset");
        calls.emplace_back("w");
        return 2.0f;
    }
    [[nodiscard]] float DeviceDepth() override {
        calls.emplace_back("depth");
        ++depth_reads;
        return 0.75f;
    }
    [[nodiscard]] std::uint32_t BeginUpdate(
        const std::uint32_t camera) override {
        Check(camera == 0x1000u, "begin camera");
        calls.emplace_back("begin");
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
    void DrawQuad(
        const std::uint32_t primitive,
        const std::array<pal4::CameraScreenVertex, 4u>& data,
        const std::uint32_t count) override {
        Check(primitive == 4u && count == 4u,
              "fog quad primitive and vertex count");
        calls.emplace_back("draw");
        drew = true;
        vertices = data;
    }
};

void CheckVertexData(const Runtime& runtime,
                     const std::uint32_t top,
                     const std::uint32_t bottom) {
    Check(runtime.drew && runtime.depth_reads == 4u,
          "four device-depth reads and one draw");
    const auto& v = runtime.vertices;
    Check(v[0].x == 0.0f && v[0].y == 0.0f &&
              v[1].x == 0.0f && v[1].y == 480.0f &&
              v[2].x == 640.0f && v[2].y == 0.0f &&
              v[3].x == 640.0f && v[3].y == 480.0f,
          "fog screen coordinates");
    Check(v[0].color == top && v[1].color == bottom &&
              v[2].color == top && v[3].color == bottom,
          "top and bottom replicated grayscale colors");
    for (const auto& vertex : v) {
        Check(vertex.z == 0.75f && vertex.reciprocal_w == 0.5f &&
                  vertex.texture_u == 0u && vertex.texture_v == 0u,
              "depth, reciprocal W and defined zero vertex tail");
    }
}

void TestBasicAndAbsoluteAmount() {
    Runtime runtime;
    Check(pal4::CameraFogOverlay::Render(
              0x1000u, 0x2000u, -0.25f, 0.0f, 1.0f,
              false, runtime) == 1u,
          "basic fog returns one");
    Check(runtime.raster_writes == std::vector<std::uint32_t>{0x2000u, 0u},
          "fog binds then clears raster");
    CheckVertexData(runtime, 0xbfbfbfbfu, 0xbfbfbfbfu);
    Check(runtime.states ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {12u, 1u}, {1u, 0u}, {6u, 0u}, {8u, 0u},
                  {29u, 8u}, {10u, 10u}, {11u, 2u},
                  {6u, 1u}, {8u, 1u}, {29u, 5u},
                  {10u, 5u}, {11u, 6u}},
          "fog device setup and restoration");
    Check(runtime.calls.front() == "round" &&
              runtime.calls[1] == "write" &&
              runtime.calls[2] == "begin" &&
              runtime.calls[runtime.calls.size() - 3u] == "state" &&
              runtime.calls[runtime.calls.size() - 2u] == "end" &&
              runtime.calls.back() == "write",
          "conversion, rendering and cleanup order");
}

void TestEnhancedColorsAndClamp() {
    Runtime runtime;
    Check(pal4::CameraFogOverlay::Render(
              0x1000u, 0x2000u, 0.75f, 2.0f, 1.0f,
              true, runtime) == 1u,
          "enhanced fog returns one");
    Check(runtime.ftol_argument == 63.0 &&
              runtime.calls[0] == "round" &&
              runtime.calls[1] == "ftol",
          "CRT conversion formula and ordering");
    CheckVertexData(runtime, 0xfefefefeu, 0x7f7f7f7fu);

    Runtime negative_extra;
    (void)pal4::CameraFogOverlay::Render(
        0x1000u, 0x2000u, 0.75f, 3.0f, 1.0f,
        true, negative_extra);
    CheckVertexData(negative_extra, 0x9f9f9f9fu, 0x40404040u);

    Runtime saturated;
    (void)pal4::CameraFogOverlay::Render(
        0x1000u, 0x2000u, 0.75f, -1.0f, 1.0f,
        true, saturated);
    CheckVertexData(saturated, 0xffffffffu, 0xffffffffu);
}

void TestBeginFailure() {
    Runtime runtime;
    runtime.begin_result = 0u;
    Check(pal4::CameraFogOverlay::Render(
              0x1000u, 0x2000u, 0.25f, 2.0f, 1.0f,
              true, runtime) == 1u,
          "failed begin still returns one");
    Check(runtime.calls == std::vector<std::string>{
              "round", "ftol", "write", "begin", "write"} &&
              runtime.raster_writes == std::vector<std::uint32_t>{
                  0x2000u, 0u} && !runtime.drew && runtime.states.empty(),
          "begin failure skips draw and state calls but unbinds raster");
}

}  // namespace

int main() {
    TestBasicAndAbsoluteAmount();
    TestEnhancedColorsAndClamp();
    TestBeginFailure();
}
