#include "pal4/pal_actor_control_movement.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlMovementRuntime {
    std::vector<std::string> calls;
    float new_axis_y = 0.0f;
    float magnitude = 0.0f;
    float set_yaw = 0.0f;
    float old_axis_y = 0.0f;

    float ComputeTransform(pal4::PalActorControlState& state,
                           std::uint32_t camera, std::uint32_t actor,
                           float& axis_y) override {
        Check(camera == 0x1000u && actor == 0x2000u,
              "transform addresses");
        old_axis_y = std::bit_cast<float>(state.words[5]);
        calls.emplace_back("compute");
        axis_y = new_axis_y;
        return magnitude;
    }
    float CameraYaw(std::uint32_t camera) override {
        Check(camera == 0x1000u, "yaw camera address");
        calls.emplace_back("yaw");
        return 20.0f;
    }
    void SetCameraYaw(std::uint32_t camera, float yaw,
                      std::uint32_t keep_look_at) override {
        Check(camera == 0x1000u && keep_look_at == 1u,
              "yaw setter arguments");
        calls.emplace_back("set");
        set_yaw = yaw;
    }
};

void TestYawBranches() {
    struct Case {
        float old_axis;
        float new_axis;
        float magnitude;
        float step;
        std::uint32_t result;
        float yaw;
        float final_axis;
        bool writes_yaw;
    };
    for (const auto& scenario : {
         Case{0.0f, -1.0f, 5.0f, 2.0f, 1u, 22.0f, -1.0f, true},
         Case{0.0f, 1.0f, 5.0f, 2.0f, 1u, 18.0f, 1.0f, true},
         Case{0.0f, -1.0f, 2.0f, 2.0f, 0u, 22.0f, 0.0f, true},
         Case{0.0f, 1.0f, 2.0f, 2.0f, 0u, 18.0f, 0.0f, true},
         Case{-1.0f, 1.0f, 5.0f, 2.0f, 0u, 15.0f, 0.0f, true},
         Case{1.0f, -1.0f, 5.0f, 2.0f, 0u, 25.0f, 0.0f, true},
         Case{0.004f, -1.0f, 5.0f, 2.0f, 1u, 22.0f, -1.0f, true},
         Case{-1.0f, -1.0f, 0.0f, 2.0f, 0u, 0.0f, -1.0f, false},
         Case{0.0f, 0.0f, 5.0f, 2.0f, 0u, 0.0f, 0.0f, false},
         Case{0.01f, 0.0f, 5.0f, 2.0f, 0u, 0.0f, 0.0f, false}}) {
        pal4::PalActorControlState state{};
        state.words.fill(0xa5a5a5a5u);
        state.words[5] = std::bit_cast<std::uint32_t>(scenario.old_axis);
        Runtime runtime;
        runtime.new_axis_y = scenario.new_axis;
        runtime.magnitude = scenario.magnitude;
        const auto result = pal4::PalActorControlMovement::UpdateYaw(
            state, 0x1000u, 0x2000u, scenario.step, runtime);
        Check(result == scenario.result &&
                  std::bit_cast<float>(state.words[5]) ==
                      scenario.final_axis &&
                  runtime.old_axis_y == scenario.old_axis,
              "movement result and sign-state write");
        Check(runtime.calls == (scenario.writes_yaw
                  ? std::vector<std::string>{"compute", "yaw", "set"}
                  : std::vector<std::string>{"compute", "yaw"}),
              "transform, yaw read and conditional angle write order");
        Check(!scenario.writes_yaw ||
                  runtime.set_yaw == scenario.yaw,
              "incremental or complete angle amount");
        Check(state.words[4] == 0xa5a5a5a5u &&
                  state.words[6] == 0xa5a5a5a5u,
              "movement preserves adjacent fields");
    }
}

void TestLinkableStub() {
    pal4::PalActorControlState state{};
    pal4::IActorControlMovementRuntime runtime;
    Check(pal4::PalActorControlMovement::UpdateYaw(
              state, 0u, 0u, 1.0f, runtime) == 0u &&
              state.words[5] == 0u,
          "default runtime composes the RenderWare-free transform stub");
}

}  // namespace

int main() {
    TestYawBranches();
    TestLinkableStub();
}
