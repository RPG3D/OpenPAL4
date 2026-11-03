#include "pal4/object_random_movement_state.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void SetFloat(pal4::ObjectRandomMovementState& state,
              std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}

float GetFloat(const pal4::ObjectRandomMovementState& state,
               std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

struct Runtime final : pal4::IObjectRandomMovementSpatialRuntime {
    pal4::BehaviorVector3 positions[2]{};
    pal4::BehaviorVector3 random{};
    std::vector<std::string> calls;
    int position_count = 0;

    pal4::BehaviorVector3 Position(
        pal4::ObjectRandomMovementState&) override {
        calls.emplace_back("position");
        Check(position_count < 2, "no more than two position queries");
        return positions[position_count++];
    }
    pal4::BehaviorVector3 RandomVector() override {
        calls.emplace_back("random");
        return random;
    }
};

void TestProximityComparison() {
    pal4::ObjectRandomMovementState state;
    Runtime runtime;
    SetFloat(state, 47u, 2.0f);
    runtime.positions[0] = {1.0f, 0.0f, 0.0f};
    Check(!pal4::ObjectRandomMovementProximity::IsOutsideTargetRadius(
              state, runtime),
          "strictly inside radius is false");
    runtime.position_count = 0;
    runtime.positions[0][0] = 2.0f;
    Check(pal4::ObjectRandomMovementProximity::IsOutsideTargetRadius(
              state, runtime),
          "equal radius is true");
    runtime.position_count = 0;
    runtime.positions[0][0] = 3.0f;
    Check(pal4::ObjectRandomMovementProximity::IsOutsideTargetRadius(
              state, runtime),
          "greater radius is true");
    runtime.position_count = 0;
    SetFloat(state, 47u, std::numeric_limits<float>::quiet_NaN());
    Check(!pal4::ObjectRandomMovementProximity::IsOutsideTargetRadius(
              state, runtime),
          "unordered threshold sets x87 C0 and returns false");
}

void TestOutsideDirection() {
    pal4::ObjectRandomMovementState state;
    Runtime runtime;
    SetFloat(state, 47u, 5.0f);
    SetFloat(state, 53u, 10.0f);
    runtime.positions[0] = {1.0f, 0.0f, 0.0f};
    runtime.positions[1] = {2.0f, 0.0f, 0.0f};
    Check(pal4::ObjectRandomMovementStateUpdate::Update(
              state, 0.25f, runtime) == 0u &&
              std::fabs(GetFloat(state, 50u) - 1.0f) < 0.0001f &&
              GetFloat(state, 51u) == 0.0f &&
              GetFloat(state, 52u) == 0.0f &&
              runtime.calls ==
                  std::vector<std::string>{"position", "position"},
          "outside target queries position twice and normalizes direction");
}

void TestZeroDirection() {
    pal4::ObjectRandomMovementState state;
    Runtime runtime;
    SetFloat(state, 47u, 0.0f);
    Check(pal4::ObjectRandomMovementStateUpdate::Update(
              state, 1.0f, runtime) == 0u &&
              state.words[50] == 0u && state.words[51] == 0u &&
              state.words[52] == 0u,
          "zero direction is copied and returns its Z bits");
}

void TestTimerAndRandomHeading() {
    pal4::ObjectRandomMovementState state;
    Runtime runtime;
    SetFloat(state, 47u, 3.0f);
    SetFloat(state, 48u, 2.0f);
    SetFloat(state, 49u, 0.5f);
    runtime.positions[0] = {1.0f, 0.0f, 0.0f};
    Check(pal4::ObjectRandomMovementStateUpdate::Update(
              state, 0.25f, runtime) == 0x100u &&
              GetFloat(state, 49u) == 0.75f &&
              runtime.calls == std::vector<std::string>{"position"},
          "inside radius accumulates timer while below period");

    runtime.calls.clear();
    runtime.position_count = 0;
    SetFloat(state, 49u, std::numeric_limits<float>::quiet_NaN());
    Check(pal4::ObjectRandomMovementStateUpdate::Update(
              state, 0.25f, runtime) == 0x100u &&
              std::isnan(GetFloat(state, 49u)) &&
              runtime.calls == std::vector<std::string>{"position"},
          "unordered timer comparison follows the C0 increment branch");

    runtime.calls.clear();
    runtime.position_count = 0;
    SetFloat(state, 49u, 2.0f);
    runtime.random = {3.0f, 99.0f, 4.0f};
    Check(pal4::ObjectRandomMovementStateUpdate::Update(
              state, 0.25f, runtime) == 0u &&
              std::fabs(GetFloat(state, 50u) - 0.6f) < 0.0001f &&
              state.words[51] == 0u &&
              std::fabs(GetFloat(state, 52u) - 0.8f) < 0.0001f &&
              state.words[49] == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"position", "random"},
          "expired timer chooses normalized horizontal random direction");
}

}  // namespace

int main() {
    TestProximityComparison();
    TestOutsideDirection();
    TestZeroDirection();
    TestTimerAndRandomHeading();
    pal4::ObjectRandomMovementState state;
    pal4::ObjectRandomMovementSpatialRuntimeStub stub;
    Check(pal4::ObjectRandomMovementStateUpdate::Update(
              state, 0.0f, stub) == 0u,
          "default runtime links");
    std::cout << "4 random movement state cases passed\n";
}
