#include "pal4/camera_render_object.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICameraRenderLifecycleRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> camera_results{0x5000u, 0x5100u};
    std::vector<std::uint32_t> raster_results{0x6000u, 0x6100u, 0x6200u};
    std::size_t camera_index{};
    std::size_t raster_index{};
    std::uint32_t pipeline_result{0x7000u};
    std::uint32_t texture_result{0x8000u};
    std::uint32_t camera_world{0x2000u};

    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x400cu, "dimension radius offset");
        calls.push_back("read-radius");
        return 7.5f;
    }
    [[nodiscard]] std::uint32_t CreateCamera(const std::uint32_t mask) override {
        calls.push_back("camera:" + std::to_string(mask));
        return camera_results.at(camera_index++);
    }
    void SetFrame(const std::uint32_t camera,
                  const std::uint32_t frame) override {
        Check(camera == 0x5000u && frame == 0x3000u, "camera frame");
        calls.push_back("frame");
    }
    void SetClip(const std::uint32_t camera, const float value) override {
        Check(camera == 0x5000u && value == 15.0f,
              "twice radius clip setup");
        calls.push_back("clip");
    }
    void UpdateTarget(const std::uint32_t camera,
                      const std::uint32_t target) override {
        Check(camera == 0x5000u && target == 0x4500u,
              "camera target setup");
        calls.push_back("target");
    }
    void AddToWorld(const std::uint32_t world,
                    const std::uint32_t camera) override {
        Check(world == 0x2000u && camera == 0x5000u,
              "camera world setup");
        calls.push_back("add-world");
    }
    [[nodiscard]] std::uint32_t CreateRaster(const std::uint32_t mask) override {
        calls.push_back("raster:" + std::to_string(mask));
        return raster_results.at(raster_index++);
    }
    void SetCameraRaster(const std::uint32_t camera,
                         const std::uint32_t raster) override {
        Check(camera == 0x5000u && raster == 0x6000u,
              "primary camera raster field");
        calls.push_back("set-camera-raster");
    }
    [[nodiscard]] std::uint32_t CreatePipeline() override {
        calls.push_back("pipeline");
        return pipeline_result;
    }
    [[nodiscard]] std::uint32_t CreateTexture(
        const std::uint32_t raster) override {
        Check(raster == 0u, "texture starts without raster");
        calls.push_back("texture");
        return texture_result;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void DetachTextureRaster(const std::uint32_t texture) override {
        Check(texture == 0x8000u, "detach texture");
        calls.push_back("detach-texture");
    }
    void DestroyTexture(const std::uint32_t texture) override {
        Check(texture == 0x8000u, "destroy texture");
        calls.push_back("destroy-texture");
    }
    void DestroyPipeline(const std::uint32_t pipeline) override {
        Check(pipeline == 0x7000u || pipeline == 0x7100u,
              "destroy pipeline pointer");
        calls.push_back("destroy-pipeline:" + std::to_string(pipeline));
    }
    [[nodiscard]] std::uint32_t GetCameraWorld(
        const std::uint32_t camera) override {
        Check(camera == 0x5000u, "camera world query");
        calls.push_back("get-camera-world");
        return camera_world;
    }
    void RemoveFromWorld(const std::uint32_t world,
                         const std::uint32_t camera) override {
        Check(world == 0x2000u && camera == 0x5000u,
              "world remove arguments");
        calls.push_back("remove-world");
    }
    void DestroyCamera(const std::uint32_t camera) override {
        Check(camera == 0x5000u || camera == 0x5100u,
              "destroy camera pointer");
        calls.push_back("destroy-camera:" + std::to_string(camera));
    }
    [[nodiscard]] std::uint32_t DestroyRaster(
        const std::uint32_t raster) override {
        calls.push_back("destroy-raster:" + std::to_string(raster));
        return 0x9000u + raster;
    }
};

std::uint32_t Initialize(pal4::CameraRenderState& state,
                         const pal4::CameraRenderOptions& options,
                         Runtime& runtime) {
    return pal4::CameraRenderObject::Initialize(
        state, 0x2000u, 0x2500u, 0x3000u, 0x4000u, 0x4500u,
        options, runtime);
}

void TestMonoInitialization() {
    Runtime runtime;
    pal4::CameraRenderState state;
    Check(Initialize(state, {2u, false}, runtime) == 1u,
          "mono camera initialization succeeds");
    Check(state.world == 0x2000u && state.clump == 0x2500u &&
              state.source_frame == 0x3000u && state.dimensions == 0x4000u &&
              state.target == 0x4500u && state.camera == 0x5000u &&
              state.auxiliary_camera == 0x5100u &&
              state.primary_raster == 0x6000u &&
              state.stereo_raster == 0u && state.effects_raster == 0x6100u &&
              state.active == 1u && state.second_pipeline == 0x7000u &&
              state.texture == 0x8000u, "mono camera fields");
    Check(runtime.calls == std::vector<std::string>{
              "camera:4", "frame", "read-radius", "clip", "target",
              "add-world", "camera:4", "raster:4", "set-camera-raster",
              "raster:4", "pipeline", "texture", "byte:32848:2",
              "byte:32849:51"}, "mono initialization order");
}

void TestStereoInitialization() {
    Runtime runtime;
    pal4::CameraRenderState state;
    Check(Initialize(state, {3u, true}, runtime) == 1u &&
              state.stereo_raster == 0x6100u &&
              state.effects_raster == 0x6200u,
          "stereo rasters stored independently");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(), "camera:8") !=
              runtime.calls.end() &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "raster:4") == 2,
          "stereo auxiliary mask is half of primary mask");
}

void TestCreationFailures() {
    {
        Runtime runtime;
        runtime.camera_results[0] = 0u;
        pal4::CameraRenderState state;
        Check(Initialize(state, {1u, false}, runtime) == 0u &&
                  state.camera == 0u && state.active == 0u &&
                  runtime.calls == std::vector<std::string>{"camera:2"},
              "first camera failure stops immediately");
    }
    {
        Runtime runtime;
        runtime.camera_results[1] = 0u;
        pal4::CameraRenderState state;
        Check(Initialize(state, {1u, false}, runtime) == 0u &&
                  state.camera == 0x5000u && state.auxiliary_camera == 0u &&
                  state.active == 0u && runtime.raster_index == 0u,
              "second camera failure retains first camera");
    }
    for (const std::size_t failed_raster : {0u, 1u, 2u}) {
        Runtime runtime;
        runtime.raster_results[failed_raster] = 0u;
        pal4::CameraRenderState state;
        const auto stereo = failed_raster != 0u;
        Check(Initialize(state, {2u, stereo}, runtime) == 0u &&
                  state.active == 0u && runtime.raster_index == failed_raster + 1u,
              "raster failure exits before activation");
    }
}

void TestTextureBranches() {
    {
        Runtime runtime;
        runtime.pipeline_result = 0u;
        pal4::CameraRenderState state;
        Check(Initialize(state, {1u, false}, runtime) == 1u &&
                  state.active == 1u && state.second_pipeline == 0u &&
                  state.texture == 0u &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "texture") == runtime.calls.end(),
              "missing pipeline still reports success");
    }
    {
        Runtime runtime;
        runtime.texture_result = 0u;
        pal4::CameraRenderState state;
        Check(Initialize(state, {1u, false}, runtime) == 0u &&
                  state.active == 1u && state.second_pipeline == 0x7000u &&
                  state.texture == 0u,
              "texture failure returns zero after activation");
    }
}

void TestCleanup() {
    Runtime runtime;
    pal4::CameraRenderState state;
    state.world = 1u;
    state.clump = 2u;
    state.source_frame = 3u;
    state.dimensions = 4u;
    state.target = 5u;
    state.active = 1u;
    state.camera = 0x5000u;
    state.auxiliary_camera = 0x5100u;
    state.primary_raster = 0x6000u;
    state.stereo_raster = 0x6100u;
    state.effects_raster = 0x6200u;
    state.current_raster = 0x6000u;
    state.first_pipeline = 0x7100u;
    state.second_pipeline = 0x7000u;
    state.texture = 0x8000u;
    state.maximum[0] = 99.0f;
    Check(pal4::CameraRenderObject::Cleanup(state, runtime) == 0xf200u,
          "cleanup forwards final raster result");
    Check(runtime.calls == std::vector<std::string>{
              "detach-texture", "destroy-texture", "destroy-pipeline:28672",
              "destroy-pipeline:28928", "get-camera-world", "remove-world",
              "destroy-camera:20480", "destroy-camera:20736",
              "destroy-raster:24576", "destroy-raster:24832",
              "destroy-raster:25088"}, "resource cleanup order");
    Check(state.world == 0u && state.clump == 0u && state.source_frame == 0u &&
              state.dimensions == 0u && state.target == 0u && state.active == 0u &&
              state.camera == 0u && state.auxiliary_camera == 0u &&
              state.primary_raster == 0u && state.stereo_raster == 0u &&
              state.effects_raster == 0u && state.current_raster == 0u &&
              state.first_pipeline == 0u && state.second_pipeline == 0u &&
              state.texture == 0u && state.maximum[0] == 99.0f,
          "cleanup clears resources but preserves bounds");

    runtime.calls.clear();
    Check(pal4::CameraRenderObject::Cleanup(state, runtime) == 0u &&
              runtime.calls.empty(), "repeated cleanup has no native calls");
}

}  // namespace

int main() {
    TestMonoInitialization();
    TestStereoInitialization();
    TestCreationFailures();
    TestTextureBranches();
    TestCleanup();
}
