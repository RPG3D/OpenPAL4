#include "pal4/camera_render_pass.h"

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

struct Geometry final : pal4::ICameraRenderGeometryRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, float> floats{
        {0x4000u, 2.0f}, {0x4004u, 4.0f}, {0x4008u, 6.0f},
        {0x400cu, 3.0f}, {0x450cu, 4.0f},
        {0x5130u, 2.0f}, {0x5134u, -2.0f}, {0x5138u, 1.0f}};
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        calls.push_back("read-word");
        Check(address == 0x5004u || address == 0x3004u,
              "geometry frame lookup address");
        return address == 0x5004u ? 0x5100u : 0x3100u;
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        return floats[address];
    }
    void WriteFloat(std::uint32_t, float) override { calls.push_back("write"); }
    [[nodiscard]] std::uint32_t GetTransform(
        const std::uint32_t frame) override {
        calls.push_back("transform");
        return frame == 0x5100u ? 0x6000u : 0x7000u;
    }
    void CopyMatrix(std::uint32_t, std::uint32_t) override {
        calls.push_back("copy");
    }
    void UpdateFrame(std::uint32_t) override { calls.push_back("update-frame"); }
    void Normalize(pal4::CameraRenderVector3&) override {
        calls.push_back("normalize");
    }
    void ApplyTransform(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {
        calls.push_back("apply");
    }
    [[nodiscard]] std::uint32_t SetClip(
        const std::uint32_t camera, const float distance) override {
        Check(camera == 0x5000u && distance == 6.0f,
              "render-scene position clip");
        calls.push_back("clip");
        return 0u;
    }
};

struct Runtime final : pal4::ICameraRenderPassRuntime {
    std::vector<std::string> calls;
    void SetProjection(const std::uint32_t texture,
                       const std::uint32_t scene,
                       const std::uint32_t camera,
                       const float strength, const float scale) override {
        Check(texture == 0x8000u && scene == 0x9000u &&
                  camera == 0x5000u && strength == 0.5f && scale == 2.0f,
              "projection arguments");
        calls.push_back("projection");
    }
    void TraverseSectors(const std::uint32_t scene,
                         const std::uint32_t callback,
                         pal4::CameraRenderState& state) override {
        Check(scene == 0x9000u && callback == 0x5fced0u &&
                  state.camera == 0x5000u,
              "visibility traversal callback");
        calls.push_back("sectors");
    }
    void TraceNoop() override { calls.push_back("noop"); }
    void RenderCollision(const std::uint32_t camera,
                         const std::uint32_t raster,
                         const std::uint32_t world,
                         const float* bounds, const float strength,
                         const float scale,
                         const std::uint32_t option) override {
        Check(camera == 0x5000u && raster == 0x6000u &&
                  world == 0x2000u && bounds[0] == 9.0f &&
                  strength == 0.5f && scale == 2.0f && option == 7u,
              "collision render arguments and frustum bounds");
        calls.push_back("collision");
    }
    void UpdateTarget(const std::uint32_t camera,
                      const std::uint32_t target) override {
        Check(camera == 0x5000u && target == 0x4000u,
              "scene target update");
        calls.push_back("target");
    }
    void ForAllAtomics(const std::uint32_t clump,
                       const std::uint32_t callback) override {
        Check(clump == 0x2500u, "scene clump");
        calls.push_back(callback == 0x5fcf80u ? "atomic-pre" :
                        callback == 0x5fd040u ? "atomic-post" :
                        "unknown-atomic");
    }
    void RenderSceneInternal(const std::uint32_t camera,
                             const std::uint32_t clump,
                             const bool normal_mode,
                             const std::uint32_t callback,
                             const std::uint32_t callback_argument) override {
        Check(camera == 0x5000u && clump == 0x2500u &&
                  callback == 0xabc0u && callback_argument == 0xdef0u,
              "internal scene callback arguments");
        calls.push_back(normal_mode ? "scene-normal" : "scene-special");
    }
    void RenderStereo(const std::uint32_t stereo_raster,
                      const std::uint32_t primary_raster,
                      const std::uint32_t auxiliary_camera) override {
        Check(stereo_raster == 0x6100u && primary_raster == 0x6000u &&
                  auxiliary_camera == 0x5100u, "stereo render arguments");
        calls.push_back("stereo");
    }
    void RenderEffects(const std::uint32_t raster,
                       const std::uint32_t effects_raster,
                       const std::uint32_t auxiliary_camera,
                       const std::uint32_t mode) override {
        Check(raster == 0x6100u && effects_raster == 0x6200u &&
                  auxiliary_camera == 0x5100u && mode == 3u,
              "post-processing arguments");
        calls.push_back("effects");
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x400cu, "fog radius address");
        calls.push_back("radius");
        return 3.0f;
    }
    [[nodiscard]] std::uint32_t RenderFog(
        const std::uint32_t auxiliary_camera,
        const std::uint32_t raster, const float strength,
        const float scale, const float radius,
        const std::uint32_t option) override {
        Check(auxiliary_camera == 0x5100u && raster == 0x6100u &&
                  strength == 0.5f && scale == 2.0f &&
                  radius == 3.0f && option == 0u, "fog render arguments");
        calls.push_back("fog");
        return 0xdeadu;
    }
};

pal4::CameraRenderState State() {
    pal4::CameraRenderState state;
    state.world = 0x2000u;
    state.clump = 0x2500u;
    state.source_frame = 0x3000u;
    state.dimensions = 0x4500u;
    state.target = 0x4000u;
    state.active = 1u;
    state.camera = 0x5000u;
    state.auxiliary_camera = 0x5100u;
    state.primary_raster = 0x6000u;
    state.stereo_raster = 0x6100u;
    state.effects_raster = 0x6200u;
    state.current_raster = 0x6000u;
    state.texture = 0x8000u;
    return state;
}

void TestRenderBranches() {
    Geometry geometry;
    Runtime runtime;
    auto state = State();
    pal4::CameraRenderGlobals globals;
    globals.frustum_scale = 2.0f;
    globals.fog_strength = 0.5f;
    globals.fog_option = 7u;

    state.active = 0u;
    pal4::CameraRenderPass::Render(
        state, 0x9000u, globals, geometry, runtime);
    Check(runtime.calls.empty() && geometry.calls.empty(),
          "inactive camera skips frustum and render");

    state.active = 1u;
    pal4::CameraRenderPass::Render(
        state, 0x9000u, globals, geometry, runtime);
    Check(state.frustum_type == 4u && runtime.calls.empty() &&
              geometry.calls == std::vector<std::string>{"read-word"},
          "disabled render still updates frustum");

    geometry.calls.clear();
    globals.render_enabled = true;
    pal4::CameraRenderPass::Render(
        state, 0x9000u, globals, geometry, runtime);
    Check(runtime.calls == std::vector<std::string>{"collision"},
          "normal render draws collision when strength nonzero");

    runtime.calls.clear();
    globals.special_mode = true;
    globals.reset_counter = 12u;
    pal4::CameraRenderPass::Render(
        state, 0x9000u, globals, geometry, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "projection", "sectors", "noop"} &&
              globals.reset_counter == 0u,
          "special render projects, traverses sectors and resets counter");

    runtime.calls.clear();
    globals.fog_strength = 0.0f;
    globals.reset_counter = 12u;
    pal4::CameraRenderPass::Render(
        state, 0x9000u, globals, geometry, runtime);
    Check(runtime.calls.empty() && globals.reset_counter == 0u,
          "zero strength still clears special-mode counter");
}

void TestRenderSceneBranches() {
    Geometry geometry;
    Runtime runtime;
    auto state = State();
    pal4::CameraRenderGlobals globals;
    globals.frustum_scale = 2.0f;
    globals.fog_strength = 0.5f;

    state.active = 0u;
    Check(pal4::CameraRenderPass::RenderScene(
              state, 0x1111u, 0xabc0u, 0xdef0u, false,
              globals, geometry, runtime) == 0u &&
              runtime.calls.empty() && geometry.calls.empty(),
          "inactive scene returns without side effects");

    state.active = 1u;
    Check(pal4::CameraRenderPass::RenderScene(
              state, 0x1111u, 0xabc0u, 0xdef0u, false,
              globals, geometry, runtime) == 0u &&
              state.current_raster == 0x6000u,
          "normal scene chooses primary raster");
    Check(runtime.calls == std::vector<std::string>{
              "target", "atomic-pre", "scene-normal", "atomic-post"},
          "normal scene callback order");
    Check(geometry.calls == std::vector<std::string>{
              "read-word", "read-word", "transform", "transform", "copy",
              "update-frame", "clip"},
          "render scene composes reconstructed position update");

    runtime.calls.clear();
    geometry.calls.clear();
    globals.special_mode = true;
    globals.stereo = true;
    globals.post_process_mode = 3u;
    globals.skip_clump_callbacks = true;
    Check(pal4::CameraRenderPass::RenderScene(
              state, 0x2222u, 0xabc0u, 0xdef0u, false,
              globals, geometry, runtime) == 0xdeadu &&
              state.current_raster == 0x6100u,
          "special scene forwards fog result and selects stereo raster");
    Check(runtime.calls == std::vector<std::string>{
              "target", "scene-special", "stereo", "effects", "radius", "fog"},
          "special scene skips clump callbacks and orders post effects");

    runtime.calls.clear();
    globals.fog_strength = 2.0f;
    Check(pal4::CameraRenderPass::RenderScene(
              state, 0x2222u, 0xabc0u, 0xdef0u, false,
              globals, geometry, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "target", "scene-special", "stereo", "effects"},
          "large fog strength and zero option skip fog call");
}

void TestRemoveFromList() {
    std::uint32_t slot = 0x1234u;
    Check(pal4::CameraRenderPass::RemoveFromList(0x5678u, slot) == 0x5678u &&
              slot == 0x1234u, "nonmatching candidate unchanged");
    Check(pal4::CameraRenderPass::RemoveFromList(0x1234u, slot) == 0u &&
              slot == 0u, "matching candidate clears list slot");
}

}  // namespace

int main() {
    TestRenderBranches();
    TestRenderSceneBranches();
    TestRemoveFromList();
}
