#include "pal4/grid_manager.h"
#include "support/random_movement_x87_reference.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool Near(const float actual, const float expected) {
    return std::abs(actual - expected) < 0.0001f;
}

class Runtime final : public pal4::IGridManagerMotionRuntime {
public:
    pal4::GridVector3 CalculateAcceleration(
        pal4::GridManagerState&, float input,
        const float delta_time) override {
        Check(input == 7.0f && delta_time == dt,
              "acceleration receives both caller arguments");
        calls.push_back("acceleration");
        return acceleration;
    }
    float ReadScalar(pal4::GridManagerState&,
                     const std::uint32_t slot) override {
        calls.push_back("scalar" + std::to_string(slot));
        if (slot == 112u)
            return acceleration_limit;
        if (slot == 80u)
            return mass;
        Check(slot == 120u, "known max-speed slot");
        return speed_limit;
    }
    pal4::GridVector3 ReadVelocity(
        pal4::GridManagerState&) override {
        calls.push_back("read_velocity");
        return velocity;
    }
    pal4::GridVector3 ReadPosition(
        pal4::GridManagerState&) override {
        calls.push_back("read_position");
        return position;
    }
    pal4::GridVector3 ReadNormal(
        pal4::GridManagerState&) override {
        calls.push_back("read_normal");
        return normal;
    }
    pal4::GridVector3 ReadOrientation(
        pal4::GridManagerState&) override {
        calls.push_back("read_orientation");
        return orientation;
    }
    void SetSpeed(pal4::GridManagerState&, const float value) override {
        calls.push_back("set_speed");
        reported_speed = value;
    }
    void SetPosition(pal4::GridManagerState&,
                     const pal4::GridVector3& value) override {
        calls.push_back("set_position");
        position = value;
    }
    void CommitVelocity(pal4::GridManagerState&,
                        const pal4::GridVector3& value,
                        const float delta_time) override {
        Check(delta_time == dt, "velocity commit receives delta time");
        calls.push_back("commit_velocity");
        committed_velocity = value;
    }
    float dt = 0.1f;
    pal4::GridVector3 acceleration{100.0f, 0.0f, 0.0f};
    float acceleration_limit = 10.0f;
    float mass = 2.0f;
    float speed_limit = 10.0f;
    pal4::GridVector3 velocity{1.0f, 0.0f, 0.0f};
    pal4::GridVector3 position{10.0f, 0.0f, 0.0f};
    pal4::GridVector3 normal{0.0f, 1.0f, 0.0f};
    pal4::GridVector3 orientation{1.0f, 0.0f, 0.0f};
    pal4::GridVector3 committed_velocity{};
    float reported_speed = 0.0f;
    std::vector<std::string> calls;
};

void PositiveStep() {
    Runtime runtime;
    pal4::GridManagerState state{};
    Check(&pal4::GridManager::UpdatePositionAndVelocity(
              state, 7.0f, runtime.dt, runtime) == &state &&
              runtime.calls == std::vector<std::string>{
                  "acceleration", "scalar112", "scalar80",
                  "read_velocity", "scalar120", "set_speed",
                  "read_position", "set_position", "commit_velocity",
                  "read_position", "read_normal", "read_normal",
                  "read_orientation", "read_normal", "read_position",
                  "read_position"},
          "motion update preserves virtual call sequence");
    Check(Near(std::bit_cast<float>(state.base_words[39]), 2.0f) &&
              Near(runtime.committed_velocity[0], 1.2f) &&
              Near(runtime.reported_speed, 1.2f) &&
              Near(runtime.position[0], 10.12f) &&
              Near(std::bit_cast<float>(state.base_words[35]), 0.06072f),
          "acceleration cap, blend, movement and final position smoothing");
}

void NegativeStepAndSpeedCap() {
    Runtime runtime;
    runtime.dt = -0.1f;
    pal4::GridManagerState state{};
    state.base_words[39] = std::bit_cast<std::uint32_t>(2.0f);
    state.base_words[35] = std::bit_cast<std::uint32_t>(4.0f);
    (void)pal4::GridManager::UpdatePositionAndVelocity(
        state, 7.0f, runtime.dt, runtime);
    Check(Near(std::bit_cast<float>(state.base_words[39]), 2.0f) &&
              Near(runtime.committed_velocity[0], 0.8f) &&
              Near(runtime.position[0], 9.92f) &&
              Near(std::bit_cast<float>(state.base_words[35]), 4.0f),
          "negative delta skips acceleration blend and final smoothing");
    Runtime capped;
    capped.acceleration = {0.0f, 0.0f, 0.0f};
    capped.velocity = {3.0f, 0.0f, 0.0f};
    capped.speed_limit = 1.0f;
    pal4::GridManagerState capped_state{};
    (void)pal4::GridManager::UpdatePositionAndVelocity(
        capped_state, 7.0f, capped.dt, capped);
    Check(Near(capped.committed_velocity[0], 1.0f) &&
              Near(capped.reported_speed, 1.0f) &&
              Near(capped.position[0], 10.1f),
          "speed above virtual limit is scaled before integration");
}

void PhysicsProjection() {
    Runtime runtime;
    runtime.position = {1.0f, 0.0f, 0.0f};
    pal4::GridManagerState state{};
    state.base_words[29] = std::bit_cast<std::uint32_t>(1.0f);
    Check(pal4::GridManager::UpdatePhysics(
              state, 0.1f, runtime) == 0u &&
              Near(std::bit_cast<float>(state.base_words[28]), -1.0f) &&
              Near(std::bit_cast<float>(state.base_words[38]), -0.4f) &&
              state.base_words[29] == 0u &&
              state.base_words[30] ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              state.base_words[32] ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.calls == std::vector<std::string>{
                  "read_position", "read_normal", "read_normal",
                  "read_orientation", "read_normal", "read_position"},
          "physics projects normal delta and smooths signed tangent length");
    runtime.calls.clear();
    Check(pal4::GridManager::UpdatePhysics(
              state, 0.0f, runtime) == 0u && runtime.calls.empty(),
          "nonpositive time skips physics sampling");
}

void MathHelpers() {
    Check(pal4::GridManager::ClampValue(-2.0f, 0.0f, 1.0f) == 0.0f &&
              pal4::GridManager::ClampValue(2.0f, 0.0f, 1.0f) == 1.0f &&
              pal4::GridManager::ClampValue(0.25f, 0.0f, 1.0f) == 0.25f,
          "clamp covers both bounds and interior");
    Check(pal4::GridManager::InterpolateVector(
              0.25f, {0.0f, 4.0f, 8.0f},
              {4.0f, 8.0f, 12.0f}) ==
              pal4::GridVector3{1.0f, 5.0f, 9.0f},
          "three-axis interpolation uses first plus scaled delta");
    Runtime runtime;
    pal4::GridManagerState state{};
    runtime.velocity = {2.0f, 3.0f, 4.0f};
    runtime.position = {10.0f, 20.0f, 30.0f};
    Check(pal4::GridManager::CalculatePosition(
              state, 0.5f, runtime) ==
              pal4::GridVector3{11.0f, 21.5f, 32.0f} &&
              runtime.calls ==
                  std::vector<std::string>{"read_velocity", "read_position"},
          "position helper samples velocity before position and integrates");
}

void CapturedFirstFrameSpeed() {
    // Pair301 helper0, first frame. The acceleration and velocity operands
    // match, but the old wide norm reports3C678E33 vs original3C678E34.
    Runtime runtime;runtime.dt=0.f;runtime.acceleration={};
    runtime.velocity={std::bit_cast<float>(0xbc081ad8u),0.f,
                      std::bit_cast<float>(0xbc3b5514u)};
    pal4::GridManagerState state{};
#if defined(_MSC_VER) && defined(_M_IX86)
    Check(std::bit_cast<std::uint32_t>(pal4::test::MovementVectorLengthPc24(
              runtime.velocity))==1013419572u,
          "captured original first-frame speed reproduced by x87 instructions");
#endif
    (void)pal4::GridManager::UpdatePositionAndVelocity(state,7.f,0.f,runtime);
    Check(std::bit_cast<std::uint32_t>(runtime.reported_speed)==1013419572u,
          "first-frame helper speed preserves original PC24 dot and square root");
}

}  // namespace

int main() {
    PositiveStep();
    NegativeStepAndSpeedCap();
    PhysicsProjection();
    MathHelpers();
    CapturedFirstFrameSpeed();
    std::cout << "Grid manager motion tests passed\n";
    return 0;
}
