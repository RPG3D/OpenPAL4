#include "pal4/camera_resource_helpers.h"

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

struct Runtime final : pal4::ICameraResourceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5004u, 0x5100u}, {0x5060u, 0x6100u},
        {0x5064u, 0u}, {0x3004u, 0x3100u}};
    std::map<std::uint32_t, float> floats{
        {0x5084u, 4.0f}, {0x5130u, 1.0f},
        {0x5134u, 0.5f}, {0x5138u, -1.0f},
        {0x5140u, 10.0f}, {0x5144u, 20.0f}, {0x5148u, 30.0f},
        {0x4000u, 2.0f}, {0x4004u, 4.0f}, {0x4008u, 6.0f}};
    std::vector<std::string> calls;
    std::uint32_t created_camera{0x5000u};
    std::uint32_t created_frame{0x5100u};
    std::uint32_t created_raster{0x6000u};

    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(floats.contains(address), "camera float offset");
        return floats[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    [[nodiscard]] std::uint32_t CreateCamera() override {
        calls.push_back("create-camera");
        return created_camera;
    }
    [[nodiscard]] std::uint32_t CreateFrame() override {
        calls.push_back("create-frame");
        return created_frame;
    }
    void AttachFrame(const std::uint32_t camera,
                     const std::uint32_t frame) override {
        Check(camera == 0x5000u, "attach frame camera");
        calls.push_back("attach-frame");
        words[camera + 4u] = frame;
    }
    [[nodiscard]] std::uint32_t CreateRaster(
        const std::uint32_t width, const std::uint32_t height,
        const std::uint32_t depth,
        const std::uint32_t type) override {
        Check(width == 16u && height == 16u && depth == 0u && type == 1u,
              "owned raster dimensions and type");
        calls.push_back("create-raster");
        return created_raster;
    }
    void SetProjection(const std::uint32_t camera,
                       const std::uint32_t type) override {
        Check(camera == 0x5000u && type == 2u,
              "camera projection type");
        calls.push_back("projection");
    }
    void DestroyFrame(const std::uint32_t frame) override {
        Check(frame == 0x5100u, "destroy owned frame");
        calls.push_back("destroy-frame");
    }
    void DestroyRaster(const std::uint32_t raster) override {
        Check(raster == 0x6000u, "destroy owned raster only");
        calls.push_back("destroy-raster");
    }
    [[nodiscard]] std::uint32_t DestroyCamera(
        const std::uint32_t camera) override {
        Check(camera == 0x5000u, "destroy camera pointer");
        calls.push_back("destroy-camera");
        return 0xbeefu;
    }
    void SetFarClip(const std::uint32_t camera,
                    const float value) override {
        Check(camera == 0x5000u && value == 15.0f,
              "far clip factor");
        calls.push_back("far");
    }
    void SetNearClip(const std::uint32_t camera,
                     const float value) override {
        Check(camera == 0x5000u && value == 10.0f * 0.001f,
              "near clip factor");
        calls.push_back("near");
    }
    void SetViewWindow(const std::uint32_t camera,
                       const float width,
                       const float height) override {
        Check(camera == 0x5000u && width == 10.0f && height == 10.0f,
              "square view window");
        calls.push_back("window");
    }
    void CopyMatrix(const std::uint32_t destination,
                    const std::uint32_t source) override {
        Check(destination == 0x5110u && source == 0x3110u,
              "64-byte frame matrix addresses");
        calls.push_back("copy-matrix");
    }
    void UpdateFrame(const std::uint32_t frame) override {
        Check(frame == 0x5100u, "update destination frame");
        calls.push_back("update-frame");
    }
    void TranslateFrame(const std::uint32_t frame,
                        const pal4::CameraRenderVector3& offset,
                        const std::uint32_t mode) override {
        Check(frame == 0x5100u && mode == 2u &&
                  offset.x == -10.0f && offset.y == -17.0f &&
                  offset.z == -22.0f,
              "target displacement and half-camera-depth correction");
        calls.push_back("translate-frame");
    }
};

void TestCreateAndDestroy() {
    Runtime runtime;
    Check(pal4::CameraResourceHelpers::Create(16u, runtime) == 0x5000u &&
              runtime.words[0x5064u] == 0x6000u,
          "camera creation stores owned raster");
    Check(runtime.calls == std::vector<std::string>{
              "create-camera", "create-frame", "attach-frame", "read:20484",
              "create-raster", "write:20580", "projection"},
          "camera creation order");

    runtime.calls.clear();
    Check(pal4::CameraResourceHelpers::Destroy(0x5000u, runtime) == 0xbeefu &&
              runtime.words[0x5064u] == 0u &&
              runtime.words[0x5060u] == 0u,
          "camera destroy clears both raster fields and forwards result");
    Check(runtime.calls == std::vector<std::string>{
              "read:20484", "attach-frame", "destroy-frame", "read:20580",
              "write:20580", "destroy-raster", "read:20576", "write:20576",
              "destroy-camera"}, "camera destroy detaches before release");

    runtime.calls.clear();
    Check(pal4::CameraResourceHelpers::Destroy(0u, runtime) == 0u &&
              runtime.calls.empty(), "null destroy is inert in C++");
}

void TestCreateFailures() {
    Runtime runtime;
    runtime.created_camera = 0u;
    Check(pal4::CameraResourceHelpers::Create(16u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"create-camera"},
          "null camera stops creation");

    runtime.calls.clear();
    runtime.created_camera = 0x5000u;
    runtime.created_frame = 0u;
    Check(pal4::CameraResourceHelpers::Create(16u, runtime) == 0u &&
              runtime.calls.back() == "destroy-camera" &&
              runtime.words[0x5004u] == 0u,
          "missing frame triggers camera cleanup");

    runtime.calls.clear();
    runtime.created_frame = 0x5100u;
    runtime.created_raster = 0u;
    Check(pal4::CameraResourceHelpers::Create(16u, runtime) == 0u &&
              runtime.calls[runtime.calls.size() - 2u] == "read:20576" &&
              runtime.calls.back() == "destroy-camera",
          "missing raster destroys created frame and camera");
}

void TestCameraTransforms() {
    Runtime runtime;
    Check(pal4::CameraResourceHelpers::SetClipAndWindow(
              0x5000u, 10.0f, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "far", "near", "window"},
          "far, near and view-window setup order");

    runtime.calls.clear();
    Check(pal4::CameraResourceHelpers::CopyFrame(
              0x5000u, 0x3000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "read:20484", "read:12292", "copy-matrix", "update-frame"},
          "copy frame uses nested pointers at plus four");

    runtime.calls.clear();
    Check(pal4::CameraResourceHelpers::UpdateTarget(
              0x5000u, 0x4000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "read:20484", "translate-frame"},
          "target update translates camera frame and returns camera");
}

}  // namespace

int main() {
    TestCreateAndDestroy();
    TestCreateFailures();
    TestCameraTransforms();
}
