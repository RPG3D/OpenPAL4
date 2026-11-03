#include "pal4/pal_actor_control_yaw_input.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlYawRuntime {
    pal4::ActorControlMouseInput input{};
    std::vector<std::string> events;
    float applied_yaw = 0.0f;
    bool update_matrix = false;

    pal4::ActorControlMouseInput ReadYawInput() override {
        events.emplace_back("input");
        return input;
    }
    float ReadCameraYaw(std::uint32_t camera) override {
        Check(camera == 0x300u, "camera yaw source");
        events.emplace_back("yaw");
        return 45.0f;
    }
    void ApplyCameraYaw(std::uint32_t camera, float yaw,
                        bool update) override {
        Check(camera == 0x300u, "camera yaw destination");
        events.emplace_back("apply");
        applied_yaw = yaw;
        update_matrix = update;
    }
};

void TestPrimaryAndSecondary() {
    Runtime runtime;
    runtime.input.primary_count = 1;
    runtime.input.primary_mode = 0x11u;
    runtime.input.secondary_count = 1;
    runtime.input.secondary_mode = 0x12u;
    Check(pal4::PalActorControlYawInput::Handle(
              0.25f, 0x300u, runtime) == 1u &&
              runtime.applied_yaw == 95.0f && runtime.update_matrix &&
              runtime.events == std::vector<std::string>{
                  "yaw", "input", "apply", "input", "input"},
          "primary yaw input has precedence and updates matrix");

    runtime = {};
    runtime.input.primary_count = 1;
    runtime.input.primary_mode = 0x10u;
    runtime.input.secondary_count = 1;
    runtime.input.secondary_mode = 0x12u;
    Check(pal4::PalActorControlYawInput::Handle(
              0.25f, 0x300u, runtime) == 1u &&
              runtime.applied_yaw == -5.0f &&
              runtime.events == std::vector<std::string>{
                  "yaw", "input", "input", "apply", "input", "input"},
          "low mode nibble gates primary and secondary subtracts yaw");
}

void TestReleaseAndIdle() {
    Runtime runtime;
    runtime.input.primary_release = 1u;
    Check(pal4::PalActorControlYawInput::Handle(
              0.5f, 0x300u, runtime) == 2u &&
              runtime.events == std::vector<std::string>{
                  "yaw", "input", "input", "input"},
          "primary release wins before secondary release check");
    runtime = {};
    runtime.input.secondary_release = 1u;
    Check(pal4::PalActorControlYawInput::Handle(
              0.5f, 0x300u, runtime) == 2u &&
              runtime.events == std::vector<std::string>{
                  "yaw", "input", "input", "input", "input"},
          "secondary release returns two");
    runtime = {};
    Check(pal4::PalActorControlYawInput::Handle(
              0.5f, 0x300u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "yaw", "input", "input", "input", "input"},
          "idle input returns zero");
}

void TestReleaseOverridesAdjustment() {
    Runtime runtime;
    runtime.input.primary_count = 1;
    runtime.input.primary_mode = 1u;
    runtime.input.secondary_release = 1u;
    Check(pal4::PalActorControlYawInput::Handle(
              0.25f, 0x300u, runtime) == 2u &&
              runtime.applied_yaw == 95.0f,
          "release overrides an earlier camera adjustment");
}

}  // namespace

int main() {
    TestPrimaryAndSecondary();
    TestReleaseAndIdle();
    TestReleaseOverridesAdjustment();
}
