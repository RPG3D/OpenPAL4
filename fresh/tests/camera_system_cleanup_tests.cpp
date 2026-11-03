#include "pal4/camera_system_cleanup.h"
#include "pal4/camera_allocation.h"
#include "pal4/camera_parameter_setters.h"
#include "pal4/camera_property_cleanup.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraAllocationRuntime,
                      public pal4::ICameraSystemObjectRuntime,
                      public pal4::ICameraParameterRuntime,
                      public pal4::ICameraPropertyCleanupRuntime,
                      public pal4::ICameraSystemCleanupRuntime {
public:
    std::array<pal4::CameraSystemObjectState, 4> objects{};
    std::vector<std::string> calls;

    static std::uint32_t Index(std::uint32_t address,
                               std::uint32_t base) {
        Check(address >= base && address < base + 4u,
              "camera cleanup address range");
        return address - base;
    }
    void DestroyDevice(std::uint32_t camera_data) override {
        calls.emplace_back("device:" + std::to_string(
            Index(camera_data, 0x5000u)));
    }
    void DestroyData(std::uint32_t camera_data) override {
        calls.emplace_back("data:" + std::to_string(
            Index(camera_data, 0x5000u)));
    }
    std::uint32_t Free(std::uint32_t camera_data) override {
        calls.emplace_back("free inner:" + std::to_string(
            Index(camera_data, 0x5000u)));
        return 0x55u;
    }
    void InitializeName(pal4::StringBufferState&) override {}
    void InitializeTasks(std::array<std::uint32_t, 4>&) override {}
    void DestroyTasks(std::array<std::uint32_t, 4>& tasks) override {
        calls.emplace_back("tasks:" + std::to_string(
            Index(tasks[1], 0x6000u)));
    }
    std::uint32_t CleanupName(pal4::StringBufferState& name) override {
        calls.emplace_back("name:" + std::to_string(
            Index(name.data, 0x7000u)));
        return 0x66u;
    }
    pal4::CameraDataState& CameraData(std::uint32_t) override {
        throw std::runtime_error("unexpected camera data lookup");
    }
    std::array<float, 3> SceneLookAt(std::uint32_t,
                                      std::uint32_t) override { return {}; }
    void UpdateTasks(std::array<std::uint32_t, 4>&,
                     float) override {}
    float MinimumDistance() override { return 0.0f; }
    float MaximumDistance() override { return 0.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&,
                               std::uint32_t) override { return 0u; }
    pal4::CameraSystemObjectState& ObjectAt(
        std::uint32_t address) override {
        const auto index = Index(address, 0x1000u);
        calls.emplace_back("object:" + std::to_string(index));
        return objects[index];
    }
    void FreeObject(std::uint32_t address) override {
        calls.emplace_back("free outer:" + std::to_string(
            Index(address, 0x1000u)));
    }
    std::uint32_t IsEmpty(pal4::CameraControllerState& state,
                          std::uint32_t offset) override {
        Check(offset == 0x10u, "property collection offset");
        for (std::uint32_t offset = 0x34u; offset <= 0x40u; offset += 4u)
            Check(state.At(offset) == 0u,
                  "all camera slots cleared before property cleanup");
        calls.emplace_back("properties");
        return 1u;
    }
    void ClearCollection(pal4::CameraControllerState&,
                         std::uint32_t) override {
        throw std::runtime_error("unexpected collection cleanup");
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraAllocation allocation(runtime);
    pal4::CameraParameterSetters parameters(runtime);
    pal4::CameraSystemObject object(runtime, allocation, parameters);
    pal4::CameraPropertyCleanup properties(runtime);
    pal4::CameraSystemCleanup cleanup(allocation, object, properties, runtime);
    pal4::CameraControllerState state;
    state.words.fill(0xdeadbeefu);
    state.At(0x30u) = 0x1000u;
    state.At(0xbcu) = 0x3044u;
    state.At(0x4u) = 0x88u;
    state.At(0x8u) = 0x99u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        state.At(0x34u + index * 4u) = 0x1000u + index;
        runtime.objects[index].camera = 0x5000u + index;
        runtime.objects[index].tasks[1] = 0x6000u + index;
        runtime.objects[index].name.data = 0x7000u + index;
    }

    Check(cleanup.Cleanup(state) == 0x99u &&
              state.At(0x30u) == 0u && state.At(0xbcu) == 0u &&
              state.At(0x8u) == 0x88u &&
              state.At(0x44u) == 0xdeadbeefu,
          "system cleanup returns old collection end and resets pointers");
    std::vector<std::string> expected;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto id = std::to_string(index);
        expected.insert(expected.end(), {
            "object:" + id, "device:" + id, "data:" + id,
            "free inner:" + id, "object:" + id, "tasks:" + id,
            "name:" + id, "free outer:" + id});
        Check(state.At(0x34u + index * 4u) == 0u &&
                  runtime.objects[index].camera == 0u,
              "outer and inner camera pointers cleared");
    }
    expected.emplace_back("properties");
    Check(runtime.calls == expected,
          "four camera objects release inner then outer resources in order");

    runtime.calls.clear();
    state.At(0x30u) = 0x1000u;
    state.At(0xbcu) = 0x3044u;
    state.At(0x8u) = 0x99u;
    Check(cleanup.Cleanup(state) == 0x99u &&
              runtime.calls == std::vector<std::string>{
                  "properties"} && state.At(0x8u) == 0x88u,
          "empty camera slots skip object release");
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
