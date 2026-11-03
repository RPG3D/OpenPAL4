#include "pal4/camera_system_initializer.h"

#include <array>
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

class Runtime final : public pal4::ICameraSystemInitializerRuntime {
public:
    std::vector<std::string> calls;
    pal4::CameraDataState first_camera;
    std::array<bool, 4> setup_results{false, true, false, true};
    unsigned allocations = 0u;
    unsigned setups = 0u;
    unsigned fail_at_allocation = 99u;

    void LogInitialization(bool complete) override {
        calls.emplace_back(complete ? "log complete" : "log begin");
    }
    pal4::CameraRenderRect RenderRect() override {
        calls.emplace_back("render rect");
        return {10, 20, 650, 500};
    }
    std::uint32_t AllocateObject(std::uint32_t bytes) override {
        Check(bytes == 0x20u, "camera object allocation size");
        calls.emplace_back("allocate");
        const auto index = allocations++;
        return index == fail_at_allocation ? 0u :
            0x1000u + index * 0x1000u;
    }
    void ConstructObject(std::uint32_t object) override {
        Check(object == 0x1000u + (allocations - 1u) * 0x1000u,
              "camera object constructor receiver");
        calls.emplace_back("construct");
    }
    bool SetupObject(std::uint32_t object,
                     const pal4::CameraSetupParameters& p) override {
        constexpr std::array<const char*, 4> names{
            "MainCamera", "ScriptCamera", "CombatCamera", "UiCamera"};
        Check(setups < names.size() &&
                  object == 0x1000u + setups * 0x1000u &&
                  std::string(p.name) == names[setups] &&
                  p.viewport == pal4::CameraRect{10, 20, 640, 480} &&
                  p.depth_raster &&
                  p.target == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
                  p.distance == 350.0f && p.angle_x == 30.0f &&
                  p.angle_y == 225.0f && p.angle_z == 0.0f,
              "four camera setup names, viewport and pose");
        calls.emplace_back("setup:" + std::to_string(setups));
        return setup_results[setups++];
    }
    pal4::CameraDataState& CameraDataForObject(
        std::uint32_t object) override {
        Check(object == 0x1000u,
              "environment callback belongs to main camera only");
        calls.emplace_back("main camera data");
        return first_camera;
    }
};

void CompleteSetup() {
    Runtime runtime;
    pal4::CameraSystemInitializer initializer(runtime);
    pal4::CameraControllerState state;
    state.words.fill(0xdeadbeefu);
    Check(initializer.Initialize(state, 0x3000u) &&
              runtime.allocations == 4u && runtime.setups == 4u &&
              runtime.calls == std::vector<std::string>{
                  "log begin", "render rect",
                  "allocate", "construct", "setup:0", "main camera data",
                  "allocate", "construct", "setup:1",
                  "allocate", "construct", "setup:2",
                  "allocate", "construct", "setup:3", "log complete"},
          "four cameras initialize in original order and return last result");
    Check(state.At(0x30u) == 0x1000u &&
              state.At(0x34u) == 0x1000u &&
              state.At(0x38u) == 0x2000u &&
              state.At(0x3cu) == 0x3000u &&
              state.At(0x40u) == 0x4000u &&
              runtime.first_camera.At(0x2bcu) == 0x5ec270u,
          "camera handles and main environment callback");
    constexpr std::array<std::uint32_t, 30> defaults{
        1130430464u, 1104150528u, 1141276672u, 1u, 1u,
        1127481344u, 1099956224u, 1132042650u, 0u, 0u,
        1131413504u, 1099956224u, 1132042650u, 0u, 0u,
        1123024896u, 1099956224u, 1132042650u, 0u, 0u,
        1127481344u, 1099956224u, 1139802112u, 0u, 0u,
        1130430464u, 1088421888u, 1133674496u, 1u, 1u};
    for (std::size_t index = 0; index < defaults.size(); ++index)
        Check(state.At(0x44u + index * 4u) == defaults[index],
              "camera controller parameter table bits");
    Check(state.At(0xbcu) == 0x3044u && state.At(0xc0u) == 0u &&
              state.At(0x28u) == 0xdeadbeefu,
          "table pointer, trailing state and untouched nested field");
}

void FailurePaths() {
    Runtime runtime;
    runtime.setup_results[3] = false;
    pal4::CameraSystemInitializer initializer(runtime);
    pal4::CameraControllerState state;
    Check(!initializer.Initialize(state, 0x3000u) &&
              runtime.setups == 4u &&
              runtime.calls.back() == "log complete",
          "earlier setup failures do not replace final setup result");

    Runtime no_memory;
    no_memory.fail_at_allocation = 0u;
    pal4::CameraSystemInitializer failed(no_memory);
    pal4::CameraControllerState empty;
    empty.words.fill(0xdeadbeefu);
    Check(!failed.Initialize(empty, 0x3000u) &&
              no_memory.calls == std::vector<std::string>{
                  "log begin", "render rect", "allocate"} &&
              empty.At(0x34u) == 0xdeadbeefu,
          "failed object allocation avoids invalid native receiver");
}

}  // namespace

int main() {
    try {
        CompleteSetup();
        FailurePaths();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
