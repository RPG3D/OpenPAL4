#include "pal4/camera_render_geometry.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICameraRenderGeometryRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x3004u, 0x3100u}, {0x5004u, 0x5100u}};
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;

    Runtime() {
        floats[0x4000u] = 7.0f;
        floats[0x4004u] = 20.0f;
        floats[0x4008u] = 30.0f;
        floats[0x400cu] = 4.0f;
        floats[0x450cu] = 4.0f;
        floats[0x5130u] = 2.0f;
        floats[0x5134u] = -4.0f;
        floats[0x5138u] = 0.0f;
        floats[0x7030u] = 10.0f;
        floats[0x7034u] = 20.0f;
        floats[0x7038u] = 30.0f;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        calls.push_back("read-word:" + std::to_string(address));
        Check(words.contains(address), "frame word address");
        return words[address];
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(floats.contains(address), "expected float address");
        return floats[address];
    }
    void WriteFloat(const std::uint32_t address,
                    const float value) override {
        calls.push_back("write:" + std::to_string(address));
        floats[address] = value;
    }
    [[nodiscard]] std::uint32_t GetTransform(
        const std::uint32_t frame) override {
        calls.push_back("transform:" + std::to_string(frame));
        Check(frame == 0x3100u || frame == 0x5100u,
              "transform frame identity");
        return frame == 0x3100u ? 0x7000u : 0x6000u;
    }
    void CopyMatrix(const std::uint32_t destination,
                    const std::uint32_t source) override {
        Check(destination == 0x6000u && source == 0x7000u,
              "matrix copy addresses");
        calls.push_back("copy-matrix");
        for (std::uint32_t i = 0u; i < 16u; ++i)
            floats[destination + i * 4u] = floats[source + i * 4u];
    }
    void UpdateFrame(const std::uint32_t frame) override {
        Check(frame == 0x5100u, "camera frame update");
        calls.push_back("update-frame");
    }
    void Normalize(pal4::CameraRenderVector3& value) override {
        calls.push_back("normalize");
        const auto length = std::sqrt(value.x * value.x + value.y * value.y +
                                      value.z * value.z);
        value.x /= length;
        value.y /= length;
        value.z /= length;
    }
    void ApplyTransform(const std::uint32_t frame,
                        const std::uint32_t matrix,
                        const std::uint32_t mode) override {
        Check(frame == 0x5100u && matrix == 0x6000u && mode == 0u,
              "apply updated transform");
        calls.push_back("apply-transform");
    }
    [[nodiscard]] std::uint32_t SetClip(
        const std::uint32_t camera, const float distance) override {
        Check(camera == 0x5000u && distance == 6.0f,
              "clip uses one and a half radius");
        calls.push_back("set-clip");
        return 0xabcdu;
    }
};

pal4::CameraRenderState State() {
    pal4::CameraRenderState state;
    state.source_frame = 0x3000u;
    state.camera = 0x5000u;
    state.target = 0x4000u;
    state.dimensions = 0x4500u;
    return state;
}

void TestFrustum() {
    Runtime runtime;
    auto state = State();
    runtime.floats[0x4000u] = 10.0f;
    Check(pal4::CameraRenderGeometry::UpdateFrustum(
              state, false, 0.5f, runtime) == 0x4000u,
          "zero final component returns x87 equal flag");
    Check(state.frustum_type == 4u &&
              state.maximum[0] == 15.0f && state.minimum[0] == 6.0f &&
              state.maximum[1] == 24.0f && state.minimum[1] == 14.0f &&
              state.maximum[2] == 34.0f && state.minimum[2] == 26.0f,
          "normal mode expands center and offsets sign-selected bound");
    Check(runtime.calls == std::vector<std::string>{"read-word:20484"},
          "frustum reads camera frame matrix once");

    runtime.calls.clear();
    Check(pal4::CameraRenderGeometry::UpdateFrustum(
              state, true, 0.5f, runtime) == 0x4000u &&
              state.maximum[0] == 11.0f && state.minimum[0] == 10.0f &&
              state.maximum[1] == 20.0f && state.minimum[1] == 18.0f &&
              state.maximum[2] == 30.0f && state.minimum[2] == 30.0f,
          "special mode starts both bounds at center");

    runtime.floats[0x5138u] = -6.0f;
    Check(pal4::CameraRenderGeometry::UpdateFrustum(
              state, true, 0.5f, runtime) == 0x100u &&
              state.minimum[2] == 27.0f,
          "negative final component returns x87 less flag");
    runtime.floats[0x5138u] = std::nanf("");
    Check(pal4::CameraRenderGeometry::UpdateFrustum(
              state, true, 0.5f, runtime) == 0x4100u &&
              std::isnan(state.minimum[2]),
          "unordered component uses lower bound and both x87 flags");
}

void TestPositionWithoutMovement() {
    Runtime runtime;
    auto state = State();
    Check(pal4::CameraRenderGeometry::UpdatePosition(
              state, false, runtime) == 0xabcdu,
          "position forwards clip result");
    Check(runtime.floats[0x6030u] == 10.0f &&
              runtime.floats[0x6034u] == 20.0f &&
              runtime.floats[0x6038u] == 30.0f,
          "source matrix copied to camera matrix");
    Check(runtime.calls == std::vector<std::string>{
              "read-word:12292", "read-word:20484", "transform:20736",
              "transform:12544", "copy-matrix", "update-frame", "set-clip"},
          "no-movement call order");
}

void TestPositionWithMovement() {
    Runtime runtime;
    auto state = State();
    Check(pal4::CameraRenderGeometry::UpdatePosition(
              state, true, runtime) == 0xabcdu,
          "moved position forwards clip result");
    Check(runtime.floats[0x6030u] == 14.0f &&
              runtime.floats[0x6034u] == 20.0f &&
              runtime.floats[0x6038u] == 30.0f,
          "normalized target displacement scales by radius");
    Check(runtime.calls == std::vector<std::string>{
              "read-word:12292", "read-word:20484", "transform:20736",
              "transform:12544", "copy-matrix", "update-frame",
              "transform:12544", "normalize", "write:24624", "write:24628",
              "write:24632", "apply-transform", "set-clip"},
          "movement call order");
}

}  // namespace

int main() {
    TestFrustum();
    TestPositionWithoutMovement();
    TestPositionWithMovement();
}
