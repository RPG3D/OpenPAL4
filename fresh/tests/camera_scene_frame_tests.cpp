#include "pal4/camera_scene_frame.h"

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

struct Runtime final : pal4::ICameraSceneFrameRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::array<pal4::CameraScreenVertex, 4u> vertices{};
    std::uint32_t begin_result = 1u;
    std::uint32_t final_state_result = 0x1234u;
    bool drew = false;

    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(address == 0x1060u, "camera raster offset");
        calls.emplace_back("raster");
        return 0x2000u;
    }
    [[nodiscard]] std::int32_t ReadSignedWord(
        const std::uint32_t address) override {
        calls.emplace_back(address == 0x200cu ? "width" : "height");
        Check(address == 0x200cu || address == 0x2010u,
              "raster dimension offsets");
        return address == 0x200cu ? 640 : 480;
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x1080u, "camera reciprocal W source");
        calls.emplace_back("w");
        return 2.0f;
    }
    [[nodiscard]] float DeviceDepth() override {
        calls.emplace_back("depth");
        return 0.75f;
    }
    void ClearCamera(const std::uint32_t camera,
                     const std::array<std::uint8_t, 4u>& color,
                     const std::uint32_t flags) override {
        Check(camera == 0x1000u &&
                  color == std::array<std::uint8_t, 4u>{255u, 255u, 255u, 0u} &&
                  flags == 3u, "clear arguments");
        calls.emplace_back("clear");
    }
    [[nodiscard]] std::uint32_t BeginUpdate(
        const std::uint32_t camera) override {
        Check(camera == 0x1000u, "begin camera");
        calls.emplace_back("begin");
        return begin_result;
    }
    void InvokeCallback(const std::uint32_t callback,
                        const std::uint32_t argument) override {
        Check(callback == 0x3010u && argument == 0x3020u,
              "callback address and argument");
        calls.emplace_back("callback");
    }
    void RenderClump(const std::uint32_t clump) override {
        Check(clump == 0x4000u, "clump argument");
        calls.emplace_back("clump");
    }
    void EndUpdate(const std::uint32_t camera) override {
        Check(camera == 0x1000u, "end camera");
        calls.emplace_back("end");
    }
    [[nodiscard]] std::uint32_t SetDeviceState(
        const std::uint32_t state, const std::uint32_t value) override {
        calls.emplace_back("state");
        states.emplace_back(state, value);
        return states.size() == 10u ? final_state_result : 0u;
    }
    void DrawScreenQuad(
        const std::uint32_t primitive,
        const std::array<pal4::CameraScreenVertex, 4u>& data,
        const std::uint32_t count) override {
        Check(primitive == 4u && count == 4u, "quad draw arguments");
        calls.emplace_back("draw");
        vertices = data;
        drew = true;
    }
};

void TestBeginFailure() {
    Runtime runtime;
    runtime.begin_result = 0u;
    Check(pal4::CameraSceneFrame::RenderInternal(
              0x1000u, 0x4000u, true, 0x3010u, 0x3020u, runtime) == 0x1000u,
          "begin failure returns camera");
    Check(runtime.calls == std::vector<std::string>{"clear", "begin"},
          "begin failure skips callback, clump, quad and end");
}

void TestRenderOptions() {
    Runtime runtime;
    Check(pal4::CameraSceneFrame::RenderInternal(
              0x1000u, 0x4000u, false, 0u, 0x3020u, runtime) == 0x1000u,
          "plain render returns camera");
    Check(runtime.calls == std::vector<std::string>{
              "clear", "begin", "clump", "end"},
          "plain render omits callback and quad");

    Runtime with_callback;
    Check(pal4::CameraSceneFrame::RenderInternal(
              0x1000u, 0x4000u, false, 0x3010u, 0x3020u,
              with_callback) == 0x1000u,
          "callback render returns camera");
    Check(with_callback.calls == std::vector<std::string>{
              "clear", "begin", "callback", "clump", "end"},
          "callback precedes clump");
}

void TestQuad() {
    Runtime runtime;
    Check(pal4::CameraSceneFrame::DrawQuad(0x1000u, runtime) == 0x1234u,
          "final state setter result forwarded");
    Check(runtime.drew, "quad is drawn");
    Check(runtime.states ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {6u, 0u}, {8u, 0u}, {1u, 0u}, {12u, 1u},
                  {11u, 1u}, {10u, 10u}, {6u, 1u}, {8u, 1u},
                  {11u, 6u}, {10u, 5u}},
          "state setup and restoration order");
    Check(runtime.calls == std::vector<std::string>{
              "raster", "width", "height", "w", "depth",
              "state", "state", "state", "state", "state", "state",
              "draw", "state", "state", "state", "state"},
          "draw occurs between state setup and restoration");

    const auto& v = runtime.vertices;
    Check(v[0].x == 0.0f && v[0].y == 0.0f &&
              v[1].x == 0.0f && v[1].y == 480.0f &&
              v[2].x == 640.0f && v[2].y == 0.0f &&
              v[3].x == 640.0f && v[3].y == 480.0f,
          "quad vertex positions");
    for (const auto& vertex : v) {
        Check(vertex.z == 0.75f && vertex.reciprocal_w == 0.5f &&
                  vertex.color == 0xffffffffu,
              "quad depth, reciprocal W and color");
        Check(vertex.texture_u == 0u && vertex.texture_v == 0u,
              "unwritten native vertex tail is zero-initialized");
    }
}

void TestRenderWithQuad() {
    Runtime runtime;
    Check(pal4::CameraSceneFrame::RenderInternal(
              0x1000u, 0x4000u, true, 0x3010u, 0x3020u, runtime) == 0x1000u,
          "quad render returns camera rather than state result");
    Check(runtime.calls.size() == 21u &&
              runtime.calls[0] == "clear" &&
              runtime.calls[1] == "begin" &&
              runtime.calls[2] == "callback" &&
              runtime.calls[3] == "clump" &&
              runtime.calls[19] == "state" &&
              runtime.calls[20] == "end",
          "callback, clump, quad and end order");
}

}  // namespace

int main() {
    TestBeginFailure();
    TestRenderOptions();
    TestQuad();
    TestRenderWithQuad();
}
