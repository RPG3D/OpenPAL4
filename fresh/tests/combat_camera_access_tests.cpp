#include "pal4/combat_camera_access.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatCameraRuntime {
    std::vector<std::string> calls;
    std::uint32_t camera_object = 0x6000u;
    std::uint32_t internal_calls = 0u;

    std::uint32_t CameraManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t FindCamera(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x5000u && name == "CombatCamera",
              "named camera lookup arguments");
        calls.emplace_back("find");
        return camera_object;
    }
    std::uint32_t InternalCamera(std::uint32_t object) override {
        Check(object == 0x6000u, "camera object argument");
        calls.emplace_back("internal:" + std::to_string(internal_calls));
        return 0x7000u + internal_calls++;
    }
    void SetPosition(std::uint32_t camera,
                     std::array<float, 3> position,
                     std::uint32_t update) override {
        Check(camera == 0x7000u &&
                  position == std::array<float, 3>{13.0f, 20.0f, -2.0f} &&
                  update == 1u,
              "combat camera position constants");
        calls.emplace_back("position");
    }
    void SetDistance(std::uint32_t camera, float distance,
                     std::uint32_t update) override {
        Check(camera == 0x7001u && distance == 452.0f && update == 1u,
              "combat camera distance constant");
        calls.emplace_back("distance");
    }
    std::uint32_t SetAngles(std::uint32_t camera,
                            std::array<float, 3> angles,
                            std::uint32_t update) override {
        Check(camera == 0x7002u &&
                  angles == std::array<float, 3>{178.0f, 14.0f, 0.0f} &&
                  update == 1u,
              "combat camera angle constants");
        calls.emplace_back("angles");
        return 0x55u;
    }
};

void SetupAndCurrent() {
    Runtime runtime;
    Check(pal4::CombatCameraAccess::Setup(runtime) == 0x55u &&
              runtime.internal_calls == 3u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "find", "internal:0", "position",
                  "internal:1", "distance", "internal:2", "angles"}),
          "setup queries internal ID for each camera update in order");

    Runtime current;
    Check(pal4::CombatCameraAccess::Current(current) == 0x7000u &&
              current.calls == (std::vector<std::string>{
                  "manager", "find", "internal:0"}),
          "current camera returns internal ID");
}

void MissingCamera() {
    Runtime runtime;
    runtime.camera_object = 0u;
    Check(pal4::CombatCameraAccess::Setup(runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "find"}),
          "setup exits when CombatCamera is absent");
    runtime.calls.clear();
    Check(pal4::CombatCameraAccess::Current(runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "find"}),
          "current camera returns null when lookup fails");
}

}  // namespace

int main() {
    try {
        SetupAndCurrent();
        MissingCamera();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
