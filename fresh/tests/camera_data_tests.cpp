#include "pal4/camera_data.h"
#include "pal4/camera_environment_probe.h"
#include "pal4/camera_obstacle_check.h"

#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraDataRuntime {
public:
    std::vector<std::string> calls;

    void InitializeNested(pal4::CameraDataState& state,
                          std::uint32_t offset) override {
        Check(offset == 0x0cu || offset == 0x230u,
              "nested initializer offset");
        Check(state.At(0) == 0u, "camera pointer reset first");
        calls.emplace_back("init:" + std::to_string(offset));
    }
    std::uint32_t NullFunctionSentinel() override {
        calls.emplace_back("sentinel");
        return 0x12345679u;
    }
    void DestroyCamera(pal4::CameraDataState& state) override {
        Check(state.At(0x94) == 0x3f800000u,
              "camera data intact before destroy");
        calls.emplace_back("destroy camera");
    }
    void DestroyNested(pal4::CameraDataState&,
                       std::uint32_t offset) override {
        Check(offset == 0x0cu, "nested destructor offset");
        calls.emplace_back("destroy nested");
    }
};

class DeviceRuntime final : public pal4::ICameraDeviceRuntime {
public:
    std::vector<std::string> calls;
    bool fail_camera = false;
    bool fail_frame = false;
    bool fail_color = false;
    bool fail_depth = false;
    std::uint32_t frame = 0u;
    std::uint32_t color = 0u;
    std::uint32_t depth = 0u;
    std::uint32_t world = 0u;

    std::uint32_t CreateCamera() override {
        calls.emplace_back("create camera");
        return fail_camera ? 0u : 0x1000u;
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create frame");
        return fail_frame ? 0u : 0x2000u;
    }
    void AttachFrame(std::uint32_t camera,
                     std::uint32_t value) override {
        Check(camera == 0x1000u, "attach camera");
        frame = value;
        calls.emplace_back(value == 0u ? "detach frame" : "attach frame");
    }
    std::uint32_t CreateRaster(std::uint32_t width,
                               std::uint32_t height,
                               std::uint32_t raster_depth,
                               std::uint32_t flags) override {
        Check(width == 640u && height == 480u && raster_depth == 0u &&
                  (flags == 1u || flags == 2u), "raster operands");
        calls.emplace_back(flags == 2u ? "create color" : "create depth");
        if ((flags == 2u && fail_color) ||
            (flags == 1u && fail_depth)) return 0u;
        return flags == 2u ? 0x3000u : 0x4000u;
    }
    void StoreRaster(std::uint32_t camera, std::uint32_t offset,
                     std::uint32_t raster) override {
        Check(camera == 0x1000u, "store raster camera");
        if (offset == 0x60u) color = raster;
        else if (offset == 0x64u) depth = raster;
        else Check(false, "store raster offset");
        calls.emplace_back(raster == 0u ? "clear raster" : "store raster");
    }
    void SetNearClip(std::uint32_t camera, float distance) override {
        Check(camera == 0x1000u && distance == 0.5f,
              "near clip operands");
        calls.emplace_back("near clip");
    }
    void SetFarClip(std::uint32_t camera, float distance) override {
        Check(camera == 0x1000u && distance == 100.0f,
              "far clip operands");
        calls.emplace_back("far clip");
    }
    std::uint32_t CameraFrame(std::uint32_t camera) override {
        Check(camera == 0x1000u, "read camera frame");
        calls.emplace_back("read frame");
        return frame;
    }
    std::uint32_t CameraRaster(std::uint32_t camera,
                               std::uint32_t offset) override {
        Check(camera == 0x1000u, "read raster camera");
        calls.emplace_back(offset == 0x60u ? "read color" : "read depth");
        return offset == 0x60u ? color : depth;
    }
    void TranslateFrame(std::uint32_t value,
                        const std::array<float, 3>& translation,
                        std::uint32_t mode) override {
        Check(value == 0x2000u &&
                  translation == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
                  mode == 0u, "initial frame translation");
        calls.emplace_back("translate frame");
    }
    std::uint32_t CameraWorld(std::uint32_t camera) override {
        Check(camera == 0x1000u, "camera world receiver");
        calls.emplace_back("read world");
        return world;
    }
    void RemoveCameraFromWorld(std::uint32_t value,
                               std::uint32_t camera) override {
        Check(value == 0x5000u && camera == 0x1000u,
              "remove world camera");
        calls.emplace_back("remove world");
    }
    void DestroyFrame(std::uint32_t value) override {
        Check(value == 0x2000u && frame == 0u,
              "frame detached before destruction");
        calls.emplace_back("destroy frame");
    }
    void DestroyRaster(std::uint32_t raster) override {
        Check((raster == 0x3000u && color == 0u) ||
                  (raster == 0x4000u && depth == 0u),
              "raster cleared before destruction");
        calls.emplace_back("destroy raster");
    }
    std::uint32_t DestroyCamera(std::uint32_t camera) override {
        Check(camera == 0x1000u, "native camera destroy receiver");
        calls.emplace_back("destroy camera");
        return 0x9000u;
    }
};

class GeometryRuntime final : public pal4::ICameraGeometryRuntime {
public:
    std::vector<std::string> calls;
    std::int32_t color_width = 640;
    std::int32_t color_height = 480;
    std::int32_t depth_width = 640;
    std::int32_t depth_height = 480;
    pal4::CameraVideoMode mode{1920, 1080, 0u};
    bool expanded = false;
    float view_x = 0.0f;
    float view_y = 0.0f;

    std::pair<std::int32_t, std::int32_t> RasterSize(
        std::uint32_t camera, std::uint32_t offset) override {
        Check(camera == 0x1000u && offset == 0x60u,
              "view window reads color raster");
        calls.emplace_back("raster size");
        return {color_width, color_height};
    }
    void SetRasterSize(std::uint32_t camera, std::uint32_t offset,
                       std::int32_t width, std::int32_t height) override {
        Check(camera == 0x1000u, "view window raster camera");
        if (offset == 0x60u) {
            color_width = width;
            color_height = height;
            calls.emplace_back("resize color");
        } else {
            Check(offset == 0x64u, "view window depth raster offset");
            depth_width = width;
            depth_height = height;
            calls.emplace_back("resize depth");
        }
    }
    std::uint32_t CurrentVideoMode() override {
        calls.emplace_back("current video mode");
        return 7u;
    }
    pal4::CameraVideoMode VideoModeInfo(std::uint32_t index) override {
        Check(index == 7u, "video mode index");
        calls.emplace_back("video mode info");
        return mode;
    }
    bool ExpandedAspect() override {
        calls.emplace_back("expanded aspect");
        return expanded;
    }
    void SetViewWindow(std::uint32_t camera, float x, float y) override {
        Check(camera == 0x1000u, "view window camera");
        view_x = x;
        view_y = y;
        calls.emplace_back("set view window");
    }
    float MinimumDistance() override {
        calls.emplace_back("minimum distance");
        return 1.0f;
    }
    float MaximumDistance() override {
        calls.emplace_back("maximum distance");
        return 50.0f;
    }
    std::uint32_t UpdateMatrix(pal4::CameraDataState& state,
                               std::uint32_t update_mode) override {
        Check(state.At(0) == 0x1000u && update_mode == 1u &&
                  state.At(0x48) == state.At(0x4c),
              "camera matrix update operands");
        calls.emplace_back("update matrix");
        return 0x8888u;
    }
};

class AnimationRuntime final : public pal4::ICameraAnimationRuntime,
                               public pal4::ICameraEnvironmentRuntime,
                               public pal4::ICameraObstacleRuntime {
public:
    std::vector<std::string> calls;
    bool environment_found = true;
    bool value_found = false;
    float environment_distance = 4.0f;
    float retrieved_distance = 2.0f;
    std::int32_t random_value = 250;

    std::int32_t Random() override {
        calls.emplace_back("random");
        return random_value;
    }
    std::uint32_t EnvironmentFrame(std::uint32_t camera) override {
        Check(camera == 0x1000u, "environment camera handle");
        if (calls.empty() || calls.back() != "environment")
            calls.emplace_back("environment");
        return environment_found ? 0x2000u : 0u;
    }
    bool ProbeAuxiliary(std::uint32_t, const pal4::CameraSegment&,
                        float&, pal4::CameraPoint&) override {
        Check(false, "no auxiliary callback in animation test");
        return false;
    }
    bool Intersect(std::uint32_t frame,
                   const pal4::CameraSegment&,
                   pal4::CameraCollision& collision) override {
        Check(frame == 0x2000u, "environment collision frame");
        collision = {0.5f, {10.0f, 20.0f,
                            30.0f - environment_distance}};
        return true;
    }
    float Magnitude(const pal4::CameraPoint& point) override {
        return std::sqrt(point[0] * point[0] + point[1] * point[1] +
                         point[2] * point[2]);
    }
    bool Intersects(pal4::CameraDataState& state,
                    std::uint32_t bounds_offset,
                    std::uint32_t point_offset,
                    std::uint32_t flags) override {
        Check(state.At(0) == 0x1000u && bounds_offset == 0x230u &&
                  point_offset == 0x2acu && flags == 0u,
              "fallback obstacle probe operands");
        if (value_found)
            state.At(0x2b8u) = std::bit_cast<std::uint32_t>(
                retrieved_distance);
        calls.emplace_back("retrieve");
        return value_found;
    }
    float MinimumDistance() override {
        calls.emplace_back("minimum");
        return 1.0f;
    }
    std::uint32_t UpdateMatrix(pal4::CameraDataState& state,
                               std::uint32_t mode) override {
        Check(state.At(0) == 0x1000u && (mode == 1u || mode == 3u),
              "animation matrix mode");
        calls.emplace_back(mode == 1u ? "animation matrix" : "matrix");
        return 0x7777u;
    }
};

void ConstructorAndDestructor() {
    Runtime runtime;
    pal4::CameraData data(runtime);
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    Check(&data.Construct(state) == &state,
          "camera constructor returns receiver");
    Check(runtime.calls == std::vector<std::string>{
              "init:12", "init:560", "sentinel"},
          "nested initialization and sentinel order");

    const std::vector<std::pair<std::size_t, std::uint32_t>> expected{
        {0x0, 0u}, {0x34, 0u}, {0x38, 1u},
        {0x3c, 0u}, {0x40, 0u}, {0x44, 0u},
        {0x48, 0x42c80000u}, {0x4c, 0x42c80000u},
        {0x8c, 0u}, {0x90, 0u}, {0x94, 0x3f800000u},
        {0x98, 0x41a00000u}, {0x9c, 0x41a00000u},
        {0xa0, 0u}, {0x1a8, 1u}, {0x1ac, 0u},
        {0x1b0, 0u}, {0x1b4, 0x42d5cccdu},
        {0x1b8, 0x41980000u}, {0x1bc, 0x416f5c29u},
        {0x2b8, 0x12345679u}, {0x2bc, 0u}};
    for (const auto& [offset, value] : expected)
        Check(state.At(offset) == value, "camera default field");
    Check(state.At(0x50) == 0xdeadbeefu &&
              state.At(0x2c0) == 0xdeadbeefu,
          "constructor leaves unobserved fields untouched");

    runtime.calls.clear();
    data.Destroy(state);
    Check(runtime.calls == std::vector<std::string>{
              "destroy camera", "destroy nested"},
          "camera teardown precedes nested cleanup");
}

void CameraDeviceCases() {
    DeviceRuntime runtime;
    pal4::CameraDevice device(runtime);
    pal4::CameraDataState state;
    Check(device.Initialize(state, 640u, 480u, true, 0.5f, 100.0f) &&
              state.At(0) == 0x1000u &&
              runtime.calls == std::vector<std::string>{
                  "create camera", "create frame", "attach frame",
                  "create color", "store raster", "create depth",
                  "store raster", "near clip", "far clip",
                  "read frame", "read color", "read depth",
                  "translate frame"},
          "camera initialization builds color and optional depth raster");
    runtime.calls.clear();
    runtime.world = 0x5000u;
    state.At(0x34) = 1u;
    Check(device.Destroy(state) == 0x9000u &&
              state.At(0) == 0u && state.At(0x34) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read world", "remove world", "read frame",
                  "detach frame", "destroy frame", "read color",
                  "clear raster", "destroy raster", "read depth",
                  "clear raster", "destroy raster", "destroy camera"},
          "camera destruction releases world, frame and rasters in order");

    runtime.calls.clear();
    Check(device.Destroy(state) == 0u && runtime.calls.empty(),
          "empty camera destroy clears state without native calls");

    DeviceRuntime failed;
    failed.fail_depth = true;
    pal4::CameraDevice failed_device(failed);
    pal4::CameraDataState failed_state;
    Check(!failed_device.Initialize(failed_state, 640u, 480u, true,
                                    0.5f, 100.0f) &&
              failed_state.At(0) == 0u &&
              failed.calls.back() == "destroy camera",
          "missing depth raster tears down partial camera");

    DeviceRuntime no_camera;
    no_camera.fail_camera = true;
    pal4::CameraDevice empty_device(no_camera);
    pal4::CameraDataState empty_state;
    empty_state.At(0x34) = 1u;
    Check(!empty_device.Initialize(empty_state, 640u, 480u, false,
                                   0.5f, 100.0f) &&
              empty_state.At(0x34) == 0u &&
              no_camera.calls == std::vector<std::string>{"create camera"},
          "failed camera creation still clears auxiliary state");

    DeviceRuntime no_frame;
    no_frame.fail_frame = true;
    pal4::CameraDevice no_frame_device(no_frame);
    pal4::CameraDataState no_frame_state;
    Check(!no_frame_device.Initialize(no_frame_state, 640u, 480u, false,
                                      0.5f, 100.0f) &&
              no_frame_state.At(0) == 0u &&
              no_frame.calls.back() == "destroy camera",
          "missing frame tears down camera without depth raster");

    DeviceRuntime no_color;
    no_color.fail_color = true;
    pal4::CameraDevice no_color_device(no_color);
    pal4::CameraDataState no_color_state;
    Check(!no_color_device.Initialize(no_color_state, 640u, 480u, false,
                                      0.5f, 100.0f) &&
              no_color_state.At(0) == 0u &&
              no_color.calls.back() == "destroy camera",
          "missing color raster tears down camera");

    DeviceRuntime color_only;
    pal4::CameraDevice color_only_device(color_only);
    pal4::CameraDataState color_only_state;
    Check(color_only_device.Initialize(color_only_state, 640u, 480u, false,
                                       0.5f, 100.0f) &&
              color_only.depth == 0u &&
              color_only.calls == std::vector<std::string>{
                  "create camera", "create frame", "attach frame",
                  "create color", "store raster", "near clip", "far clip",
                  "read frame", "read color", "translate frame"},
          "color-only camera skips depth creation and validation");
}

void CameraGeometryCases() {
    GeometryRuntime runtime;
    pal4::CameraGeometry geometry(runtime);
    pal4::CameraDataState state;
    pal4::CameraViewportGlobals globals;
    geometry.AdjustViewWindow(state, globals, nullptr, 1.0f, 9);
    Check(runtime.calls.empty(), "null camera skips viewport services");

    state.At(0) = 0x1000u;
    geometry.AdjustViewWindow(state, globals, nullptr, 1.0f, 9);
    Check(globals.original_width == 640 &&
              globals.original_height == 480 &&
              globals.original_x == 0 && globals.original_y == 0 &&
              globals.default_rect == pal4::CameraRect{0, 0, 640, 480} &&
              runtime.color_width == 640 && runtime.depth_width == 640 &&
              runtime.view_x == 1.0f && runtime.view_y == 0.75f &&
              runtime.calls == std::vector<std::string>{
                  "raster size", "current video mode", "video mode info",
                  "raster size", "resize color", "resize depth",
                  "expanded aspect", "set view window"},
          "default viewport captures initial raster size and landscape aspect");

    runtime.calls.clear();
    pal4::CameraRect portrait{5, 7, 600, 900};
    runtime.expanded = true;
    geometry.AdjustViewWindow(state, globals, &portrait, 0.5f, 0);
    Check(portrait == pal4::CameraRect{5, 7, 600, 900} &&
              runtime.color_width == 600 && runtime.color_height == 900 &&
              runtime.depth_width == 600 && runtime.depth_height == 900 &&
              std::fabs(runtime.view_x - 0.44444448f) < 0.00001f &&
              std::fabs(runtime.view_y - 0.6666667f) < 0.00001f &&
              state.At(0x4) == std::bit_cast<std::uint32_t>(runtime.view_x) &&
              state.At(0x8) == std::bit_cast<std::uint32_t>(runtime.view_y),
          "portrait viewport writes both rasters and expanded view window");

    runtime.calls.clear();
    runtime.mode.flags = 1u;
    geometry.AdjustViewWindow(state, globals, &portrait, 1.0f, 0);
    Check(portrait == pal4::CameraRect{0, 0, 1920, 1080} &&
              runtime.color_width == 1920 && runtime.depth_height == 1080 &&
              globals.original_width == 640 &&
              runtime.calls[0] == "current video mode",
          "video mode flag overrides supplied rectangle without recapturing original size");

    runtime.calls.clear();
    runtime.mode.flags = 0u;
    pal4::CameraRect invalid{0, 0, -1, 900};
    geometry.AdjustViewWindow(state, globals, &invalid, 1.0f, 0);
    Check(runtime.calls == std::vector<std::string>{
              "current video mode", "video mode info"},
          "invalid viewport exits before raster or view-window writes");

    runtime.calls.clear();
    const std::array<float, 3> target{3.0f, 4.0f, 5.0f};
    Check(geometry.SetParameters(state, target, 10.0f, 20.0f, 30.0f,
                                 0.25f) == 0x8888u &&
              state.At(0x74) == std::bit_cast<std::uint32_t>(3.0f) &&
              state.At(0x78) == std::bit_cast<std::uint32_t>(4.0f) &&
              state.At(0x7c) == std::bit_cast<std::uint32_t>(5.0f) &&
              state.At(0x3c) == std::bit_cast<std::uint32_t>(10.0f) &&
              state.At(0x40) == std::bit_cast<std::uint32_t>(20.0f) &&
              state.At(0x44) == std::bit_cast<std::uint32_t>(30.0f) &&
              state.At(0x4c) == std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.calls == std::vector<std::string>{
                  "minimum distance", "maximum distance", "update matrix"},
          "camera parameters clamp below minimum and update matrix");

    runtime.calls.clear();
    Check(geometry.SetParameters(state, target, 0.0f, 0.0f, 0.0f,
                                 90.0f) == 0x8888u &&
              state.At(0x4c) == std::bit_cast<std::uint32_t>(50.0f),
          "camera distance clamps above maximum");
    runtime.calls.clear();
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    Check(geometry.SetParameters(state, target, 0.0f, 0.0f, 0.0f,
                                 nan) == 0x8888u &&
              std::isnan(std::bit_cast<float>(state.At(0x4c))),
          "unordered camera distance is preserved by x87 comparisons");

    runtime.calls.clear();
    state.At(0) = 0u;
    Check(!geometry.SetParameters(state, target, 1.0f, 2.0f, 3.0f,
                                  4.0f).has_value() && runtime.calls.empty(),
          "absent camera does not update parameters or matrix");
}

void CameraAnimationCases() {
    AnimationRuntime runtime;
    pal4::CameraEnvironmentProbe environment(runtime);
    pal4::CameraObstacleCheck obstacle(runtime);
    pal4::CameraAnimation animation(runtime, environment, obstacle);
    pal4::CameraDataState state;
    Check(!animation.Update(state, 0.5f).has_value() &&
              runtime.calls.empty(),
          "absent camera skips animation callbacks");

    state.At(0) = 0x1000u;
    Check(animation.Update(state, 0.5f) == 0u &&
              runtime.calls.empty(),
          "disabled camera animations make no runtime calls");

    state.At(0x1a8) = 1u;
    state.At(0x38) = 3u;
    state.At(0x48) = std::bit_cast<std::uint32_t>(2.0f);
    state.At(0x4c) = std::bit_cast<std::uint32_t>(10.0f);
    state.At(0x1b4) = std::bit_cast<std::uint32_t>(4.0f);
    state.At(0x74u) = std::bit_cast<std::uint32_t>(10.0f);
    state.At(0x78u) = std::bit_cast<std::uint32_t>(20.0f);
    state.At(0x7cu) = std::bit_cast<std::uint32_t>(30.0f);
    state.At(0x84u) = std::bit_cast<std::uint32_t>(1.0f);
    state.At(0x58u) = std::bit_cast<std::uint32_t>(1.0f);
    state.At(0x1bcu) = std::bit_cast<std::uint32_t>(2.0f);
    runtime.calls.clear();
    Check(animation.Update(state, 0.5f) == 0x7777u &&
              std::bit_cast<float>(state.At(0x1c4u)) == 22.0f &&
              std::bit_cast<float>(state.At(0x1d4u)) == 26.0f &&
              state.At(0x48) == std::bit_cast<std::uint32_t>(4.0f) &&
              runtime.calls == std::vector<std::string>{
                  "environment", "minimum", "matrix"},
          "moving camera uses environment distance and updates matrix");

    runtime.calls.clear();
    runtime.environment_distance = 40.0f;
    Check(animation.Update(state, 0.5f) == 0x7777u &&
              state.At(0x48) == state.At(0x4c),
          "environment distance above target is clamped");

    state.At(0x48) = std::bit_cast<std::uint32_t>(2.0f);
    runtime.calls.clear();
    runtime.environment_distance = 0.25f;
    Check(animation.Update(state, 0.5f) == 0x7777u &&
              state.At(0x48) == std::bit_cast<std::uint32_t>(1.0f),
          "environment distance below game minimum is clamped");

    state.At(0x48) = std::bit_cast<std::uint32_t>(2.0f);
    runtime.calls.clear();
    runtime.environment_found = false;
    runtime.value_found = true;
    runtime.retrieved_distance = 0.5f;
    Check(animation.Update(state, 0.5f) == 0x7777u &&
              state.At(0x48) == std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.calls == std::vector<std::string>{
                  "environment", "retrieve", "minimum", "matrix"},
          "fallback value replaces advanced distance then clamps to minimum");

    state.At(0x48) = std::bit_cast<std::uint32_t>(2.0f);
    runtime.calls.clear();
    runtime.value_found = false;
    Check(animation.Update(state, 0.5f) == 0x7777u &&
              state.At(0x48) == std::bit_cast<std::uint32_t>(4.0f),
          "failed fallback probe keeps advanced distance");

    state.At(0x48) = state.At(0x4c);
    runtime.calls.clear();
    Check(animation.Update(state, 0.5f) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "environment"},
          "settled camera returns failed environment result directly");

    runtime.calls.clear();
    runtime.environment_found = true;
    runtime.environment_distance = 5.0f;
    Check(animation.Update(state, 0.5f) == 0x100u &&
              state.At(0x48) == std::bit_cast<std::uint32_t>(5.0f),
          "settled camera preserves environment distance below target");

    runtime.calls.clear();
    runtime.environment_distance = 12.0f;
    state.At(0x48) = state.At(0x4c);
    Check(animation.Update(state, 0.5f) == 0u &&
              state.At(0x48) == state.At(0x4c),
          "settled camera caps environment distance at target");
}

void PositionAnimationCases() {
    AnimationRuntime runtime;
    pal4::CameraEnvironmentProbe environment(runtime);
    pal4::CameraObstacleCheck obstacle(runtime);
    pal4::CameraAnimation animation(runtime, environment, obstacle);
    pal4::CameraDataState state;
    state.At(0) = 0x1000u;
    Check(animation.AnimatePosition(state, 0.5f) == 0u &&
              runtime.calls.empty(),
          "inactive position animation returns zero");

    state.At(0x8cu) = 1u;
    state.At(0x94u) = std::bit_cast<std::uint32_t>(0.0f);
    Check(!animation.AnimatePosition(state, 0.5f).has_value() &&
              runtime.calls.empty(),
          "nonpositive duration returns unspecified native status");

    state.At(0x94u) = std::bit_cast<std::uint32_t>(2.0f);
    state.At(0x90u) = std::bit_cast<std::uint32_t>(0.0f);
    state.At(0x9cu) = std::bit_cast<std::uint32_t>(1.0f);
    state.At(0xa0u) = std::bit_cast<std::uint32_t>(0.0f);
    state.At(0x98u) = std::bit_cast<std::uint32_t>(10.0f);
    state.At(0xa4u) = std::bit_cast<std::uint32_t>(6.0f);
    Check(animation.AnimatePosition(state, 0.5f) == 0x7777u &&
              state.At(0x8cu) == 1u &&
              std::bit_cast<float>(state.At(0x90u)) == 0.5f &&
              std::bit_cast<float>(state.At(0xa0u)) == 0.5f &&
              std::fabs(std::bit_cast<float>(state.At(0x78u)) -
                        9.672846f) < 0.00001f &&
              runtime.calls == std::vector<std::string>{
                  "random", "animation matrix"},
          "position animation advances phase and fades sine/random offset");

    runtime.calls.clear();
    Check(animation.AnimatePosition(state, 2.0f) == 0x7777u &&
              state.At(0x8cu) == 0u &&
              std::bit_cast<float>(state.At(0x90u)) == 2.0f &&
              std::bit_cast<float>(state.At(0x78u)) == 6.0f &&
              runtime.calls == std::vector<std::string>{
                  "random", "animation matrix"},
          "completion clamps elapsed time and restores baseline Y");

    runtime.calls.clear();
    state.At(0x1a8u) = 0u;
    Check(animation.Update(state, 0.5f) == 0u && runtime.calls.empty(),
          "frame update calls reconstructed position animation after completion");

    // Pair272's late quake: original 5EA3C3..5EA411 at PC24 gives
    // 4383BCE5; passing the unrounded double phase to sine gives 4383BCE6.
    runtime.random_value = 24749;
    state.At(0x8cu) = 1u;
    state.At(0x90u) = std::bit_cast<std::uint32_t>(7.39503288269043f);
    state.At(0x94u) = std::bit_cast<std::uint32_t>(40.0f);
    state.At(0x98u) = std::bit_cast<std::uint32_t>(10.0f);
    state.At(0x9cu) = std::bit_cast<std::uint32_t>(2.0f);
    state.At(0xa0u) = std::bit_cast<std::uint32_t>(14.79006576538086f);
    state.At(0xa4u) = std::bit_cast<std::uint32_t>(258.3300476074219f);
    Check(animation.AnimatePosition(state, 0.017f) == 0x7777u &&
              state.At(0x90u) == 0x40ed2f60u &&
              state.At(0xa0u) == 0x416d2f60u &&
              state.At(0x78u) == 0x4383bce5u,
          "position quake rounds phase and each game operation at PC24");

    runtime.calls.clear();
    state.At(0x8cu) = 1u;
    state.At(0x90u) = std::bit_cast<std::uint32_t>(0.0f);
    state.At(0xa0u) = std::bit_cast<std::uint32_t>(0.0f);
    Check(animation.Update(state, 0.5f) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "random", "animation matrix"},
          "frame update executes reconstructed position animation first");
}

void OriginalM05DistanceAdvance() {
    AnimationRuntime runtime;
    runtime.environment_found=false;
    pal4::CameraEnvironmentProbe environment(runtime);
    pal4::CameraObstacleCheck obstacle(runtime);
    pal4::CameraAnimation animation(runtime,environment,obstacle);
    pal4::CameraDataState state;
    state.At(0)=0x1000u;state.At(0x38u)=1u;state.At(0x1a8u)=1u;
    state.At(0x48u)=0x429af6e8u; // actual M05 current distance
    state.At(0x4cu)=0x44068000u; // requested538
    state.At(0x1b4u)=0x42d5cccdu; // original106.9 speed
    // IDA5E94A0..5E94AA: PC24 fmul then fadd, unlike a double expression.
    Check(animation.Update(state,0.041f)==0x7777u &&
              state.At(0x48u)==0x42a3baf4u && state.At(0x4cu)==0x44068000u,
          "M05 distance advance rounds the original multiply before adding current distance");
}

}  // namespace

int main() {
    try {
        ConstructorAndDestructor();
        CameraDeviceCases();
        CameraGeometryCases();
        CameraAnimationCases();
        OriginalM05DistanceAdvance();
        PositionAnimationCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
