#include "pal4/camera_system_object.h"
#include "pal4/camera_allocation.h"
#include "pal4/camera_parameter_setters.h"
#include "pal4/camera_environment_probe.h"
#include "pal4/camera_obstacle_check.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraSystemObjectRuntime,
                      public pal4::ICameraAllocationRuntime,
                      public pal4::ICameraParameterRuntime {
public:
    std::vector<std::string> calls;
    pal4::CameraDataState camera_data;

    void InitializeName(pal4::StringBufferState& name) override {
        Check(name.data == 0xdeadbeefu,
              "constructor leaves name storage to string initializer");
        name = {0x2000u, 0u, 0x20u};
        calls.emplace_back("initialize name");
    }
    void InitializeTasks(std::array<std::uint32_t, 4>& tasks) override {
        Check(tasks[0] == 0xdeadbeefu,
              "task initializer sees untouched task state");
        tasks = {0u, 0x3000u, 0u, 1u};
        calls.emplace_back("initialize tasks");
    }
    void DestroyDevice(std::uint32_t camera_data) override {
        Check(camera_data == 0x1000u,
              "camera device teardown receives allocation");
        calls.emplace_back("destroy device");
    }
    void DestroyData(std::uint32_t camera_data) override {
        Check(camera_data == 0x1000u,
              "camera data cleanup receives allocation");
        calls.emplace_back("destroy data");
    }
    std::uint32_t Free(std::uint32_t camera_data) override {
        Check(camera_data == 0x1000u,
              "camera memory release receives allocation");
        calls.emplace_back("free camera");
        return 0x77u;
    }
    void DestroyTasks(std::array<std::uint32_t, 4>& tasks) override {
        Check(tasks[1] == 0x3000u,
              "task container destroyed after camera resource");
        calls.emplace_back("destroy tasks");
    }
    std::uint32_t CleanupName(pal4::StringBufferState& name) override {
        Check(name.data == 0x2000u,
              "string cleaned after task container");
        calls.emplace_back("cleanup name");
        return 0x88u;
    }
    pal4::CameraDataState& CameraData(std::uint32_t camera) override {
        Check(camera == 0x1000u, "scene parameter camera allocation");
        calls.emplace_back("resolve camera");
        return camera_data;
    }
    std::array<float, 3> SceneLookAt(
        std::uint32_t scene, std::uint32_t selector) override {
        Check(scene == 0x5678u && selector == 3u,
              "scene vector lookup operands");
        calls.emplace_back("scene look at");
        return {10.0f, 20.0f, 30.0f};
    }
    void UpdateTasks(std::array<std::uint32_t, 4>& tasks,
                     float delta) override {
        Check(tasks[1] == 0x3000u && delta == 0.25f,
              "camera task update operands");
        calls.emplace_back("update tasks");
    }
    float MinimumDistance() override { return 0.0f; }
    float MaximumDistance() override { return 100.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState& state,
                               std::uint32_t mode) override {
        Check(mode == 3u && state.At(0x74u) ==
                  std::bit_cast<std::uint32_t>(10.0f),
              "scene look-at matrix update mode and target");
        calls.emplace_back("update matrix");
        return 0x88u;
    }
};

class SilentAnimationRuntime final : public pal4::ICameraAnimationRuntime,
                                     public pal4::ICameraEnvironmentRuntime,
                                     public pal4::ICameraObstacleRuntime {
public:
    std::int32_t Random() override { return 0; }
    float MinimumDistance() override { return 0.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&,
                               std::uint32_t) override { return 0u; }
    std::uint32_t EnvironmentFrame(std::uint32_t) override { return 0u; }
    bool ProbeAuxiliary(std::uint32_t, const pal4::CameraSegment&,
                        float&, pal4::CameraPoint&) override { return false; }
    bool Intersect(std::uint32_t, const pal4::CameraSegment&,
                   pal4::CameraCollision&) override { return false; }
    float Magnitude(const pal4::CameraPoint&) override { return 0.0f; }
    bool Intersects(pal4::CameraDataState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override { return false; }
};

void Cases() {
    Runtime runtime;
    pal4::CameraAllocation allocation(runtime);
    pal4::CameraParameterSetters parameters(runtime);
    pal4::CameraSystemObject object(runtime, allocation, parameters);
    pal4::CameraSystemObjectState state;
    state.camera = 0xdeadbeefu;
    state.name = {0xdeadbeefu, 0xdeadbeefu, 0xdeadbeefu};
    state.tasks.fill(0xdeadbeefu);
    Check(&object.Construct(state) == &state && state.camera == 0u &&
              state.name.data == 0x2000u && state.name.length == 0u &&
              state.name.capacity == 0x20u && state.tasks[1] == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "initialize name", "initialize tasks"},
          "camera system object constructor order and offsets");

    SilentAnimationRuntime animation_runtime;
    pal4::CameraEnvironmentProbe environment(animation_runtime);
    pal4::CameraObstacleCheck obstacle(animation_runtime);
    pal4::CameraAnimation animation(animation_runtime, environment, obstacle);
    runtime.calls.clear();
    Check(!object.Update(state, animation, 0.25f).has_value() &&
              runtime.calls.empty(),
          "missing camera skips task and animation updates");

    state.camera = 0x1000u;
    runtime.calls.clear();
    Check(object.InternalId(state) == 0x1000u && runtime.calls.empty(),
          "internal ID getter returns first object word");
    runtime.camera_data.At(0) = 0x5000u;
    Check(!object.SetLookAtFromScene(state, 3u) &&
              runtime.calls == std::vector<std::string>{"resolve camera"},
          "empty scene pointer skips target lookup and matrix update");
    runtime.calls.clear();
    Check(object.SetSceneParameter(state, 0x5678u) == 0x5678u &&
              runtime.camera_data.At(0x34u) == 0x5678u &&
              runtime.calls == std::vector<std::string>{"resolve camera"},
          "scene parameter thunk writes camera data and forwards argument");
    runtime.calls.clear();
    Check(object.SetLookAtFromScene(state, 3u) &&
              runtime.camera_data.At(0x74u) ==
                  std::bit_cast<std::uint32_t>(10.0f) &&
              runtime.camera_data.At(0x78u) ==
                  std::bit_cast<std::uint32_t>(20.0f) &&
              runtime.camera_data.At(0x7cu) ==
                  std::bit_cast<std::uint32_t>(30.0f) &&
              runtime.calls == std::vector<std::string>{
                  "resolve camera", "scene look at", "update matrix"},
          "scene target getter feeds reconstructed camera setter");
    runtime.calls.clear();
    Check(object.Update(state, animation, 0.25f) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "update tasks", "resolve camera"},
          "game loop updates tasks before reconstructed camera animation");
    runtime.calls.clear();
    runtime.camera_data.At(0x3cu) = std::bit_cast<std::uint32_t>(10.0f);
    runtime.camera_data.At(0x40u) = std::bit_cast<std::uint32_t>(20.0f);
    runtime.camera_data.At(0x44u) = std::bit_cast<std::uint32_t>(30.0f);
    Check(object.AdjustAngles(state, {5.0f, -5.0f, 20.0f}, 3u) ==
              0x88u &&
              runtime.camera_data.At(0x3cu) ==
                  std::bit_cast<std::uint32_t>(15.0f) &&
              runtime.camera_data.At(0x40u) ==
                  std::bit_cast<std::uint32_t>(15.0f) &&
              runtime.camera_data.At(0x44u) ==
                  std::bit_cast<std::uint32_t>(50.0f) &&
              runtime.calls == std::vector<std::string>{
                  "resolve camera", "update matrix"},
          "angle delta wrapper uses reconstructed normalization and mode");
    runtime.calls.clear();
    runtime.camera_data.At(0x48u) = std::bit_cast<std::uint32_t>(5.0f);
    Check(object.AdjustDistance(state, 2.0f, 3u) == 0x88u &&
              runtime.camera_data.At(0x48u) ==
                  std::bit_cast<std::uint32_t>(7.0f) &&
              runtime.camera_data.At(0x4cu) ==
                  std::bit_cast<std::uint32_t>(7.0f) &&
              runtime.calls == std::vector<std::string>{
                  "resolve camera", "update matrix"},
          "distance delta wrapper uses current distance and matrix mode");
    runtime.calls.clear();
    Check(object.AdjustAnglesAndDistance(
              state, {1.0f, 2.0f, 3.0f}, 4.0f, 3u) == 0x88u &&
              runtime.camera_data.At(0x3cu) ==
                  std::bit_cast<std::uint32_t>(16.0f) &&
              runtime.camera_data.At(0x40u) ==
                  std::bit_cast<std::uint32_t>(17.0f) &&
              runtime.camera_data.At(0x44u) ==
                  std::bit_cast<std::uint32_t>(53.0f) &&
              runtime.camera_data.At(0x48u) ==
                  std::bit_cast<std::uint32_t>(11.0f) &&
              runtime.camera_data.At(0x4cu) ==
                  std::bit_cast<std::uint32_t>(11.0f) &&
              runtime.calls == std::vector<std::string>{
                  "resolve camera", "update matrix"},
          "combined delta wrapper forwards angles and distance once");
    runtime.calls.clear();
    Check(object.Destroy(state) == 0x88u && state.camera == 0u &&
              runtime.calls == std::vector<std::string>{
                  "destroy device", "destroy data", "free camera",
                  "destroy tasks", "cleanup name"},
          "camera system object destructor order and return");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
