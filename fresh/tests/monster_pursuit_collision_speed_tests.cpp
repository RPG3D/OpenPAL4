#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_pursuit_collision_speed.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using State = pal4::MonsterBehaviorManagerState;

void SetFloat(State& state, std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

State InitialState() {
    State state{};
    SetFloat(state, 0xacu, 1.0f);
    SetFloat(state, 0xb0u, 2.0f);
    SetFloat(state, 0xb4u, 3.0f);
    state.words[0x654u / 4u] = 0x8000u;
    return state;
}

struct Runtime final : pal4::IMonsterPursuitReturnRuntime {
    std::vector<std::string> calls;
    pal4::MonsterPursuitTarget target{true, {4.0f, 5.0f, 6.0f}};
    std::uint32_t path{0x9000u};
    bool accepted{true};
    float speed{50.0f};
    std::array<float, 3> observed_manager{};
    std::array<float, 3> observed_target{};
    float observed_elapsed{};
    float observed_speed{};
    std::uint32_t speed_result{0x1234u};
    std::uint32_t reset_result{0x5678u};

    pal4::MonsterPursuitTarget Target(std::uint32_t address) override {
        Check(address == 0x7000u, "target address");
        calls.emplace_back("target");
        return target;
    }
    std::uint32_t FindPursuitPath(float elapsed,
                                  std::array<float, 3>& position) override {
        calls.emplace_back("path");
        observed_elapsed = elapsed;
        Check(position == target.position ||
                  position == std::array<float, 3>{8.0f, 9.0f, 10.0f},
              "path receives target or saved position");
        position[2] = 7.0f;
        return path;
    }
    bool ApplyPursuitPath(std::uint32_t movement,
        const std::array<float, 3>& manager_position,
        const std::array<float, 3>& target_position,
        std::uint32_t path_address, std::uint32_t argument) override {
        Check(movement == 0x5564u && path_address == path &&
                  argument == 0u, "movement path dispatch arguments");
        calls.emplace_back("apply-path");
        observed_manager = manager_position;
        observed_target = target_position;
        return accepted;
    }
    float MaxPursuitSpeed(std::uint32_t source, float fallback) override {
        Check(source == 0x8000u && fallback == 50.0f,
              "MST speed lookup and default");
        calls.emplace_back("speed-config");
        return speed;
    }
    std::uint32_t SetPursuitSpeed(std::uint32_t movement,
                                  float value) override {
        Check(movement == 0x5564u, "speed movement subobject");
        calls.emplace_back("set-speed");
        observed_speed = value;
        return speed_result;
    }
    std::uint32_t ResetPursuit(State& state,
                                std::uint32_t address) override {
        Check(address == 0x5000u, "reset manager address");
        calls.emplace_back("reset");
        state.words[0x6e0u / 4u] = 0u;
        return reset_result;
    }
};

bool Process(const State& state, Runtime& runtime,
             std::uint32_t target = 0x7000u) {
    return pal4::MonsterPursuitCollisionSpeed::Process(
        state, 0x5000u, 0.25f, target, runtime);
}

void TargetAndPathGates() {
    const auto state = InitialState();
    Runtime runtime;
    Check(!Process(state, runtime, 0u) && runtime.calls.empty(),
          "null target skips all services");
    runtime.target.active = false;
    Check(!Process(state, runtime) &&
              runtime.calls == std::vector<std::string>{"target"},
          "inactive target skips path lookup");

    Runtime no_path;
    no_path.path = 0u;
    Check(!Process(state, no_path) &&
              no_path.calls == std::vector<std::string>{"target", "path"},
          "missing path skips movement dispatch");

    Runtime rejected;
    rejected.accepted = false;
    Check(!Process(state, rejected) &&
              rejected.calls == std::vector<std::string>{
                  "target", "path", "apply-path"},
          "movement rejection skips speed lookup");
}

void SuccessfulPathAndSpeed() {
    const auto state = InitialState();
    for (const auto speed : {50.0f, 12.5f}) {
        Runtime runtime;
        runtime.speed = speed;
        Check(Process(state, runtime) &&
                  runtime.observed_elapsed == 0.25f &&
                  runtime.observed_manager == std::array<float, 3>{
                      1.0f, 2.0f, 3.0f} &&
                  runtime.observed_target == std::array<float, 3>{
                      4.0f, 5.0f, 7.0f} &&
                  runtime.observed_speed == speed &&
                  runtime.calls == std::vector<std::string>{
                      "target", "path", "apply-path",
                      "speed-config", "set-speed"},
              "accepted path forwards modified vector and configured speed");
    }
}

void DefaultStub() {
    const auto state = InitialState();
    pal4::MonsterPursuitSpeedRuntimeStub runtime;
    Check(!pal4::MonsterPursuitCollisionSpeed::Process(
              state, 0x5000u, 0.25f, 0u, runtime) &&
              !pal4::MonsterPursuitCollisionSpeed::Process(
                  state, 0x5000u, 0.25f, 0x7000u, runtime),
          "external path and movement services have a linkable stub");
}

void ReturnStateBranches() {
    auto initial = InitialState();
    initial.words[0x6e4u / 4u] = 0x7000u;
    initial.words[0x6f8u / 4u] = 0x40200000u;
    SetFloat(initial, 0x6e8u, 8.0f);
    SetFloat(initial, 0x6ecu, 9.0f);
    SetFloat(initial, 0x6f0u, 10.0f);

    auto no_path_state = initial;
    Runtime no_path;
    no_path.path = 0u;
    Check(pal4::MonsterPursuitReturnState::Enter(
              no_path_state, 0x5000u, 0.25f, no_path) == 0u &&
              no_path_state.words[0x6e0u / 4u] == 2u &&
              no_path_state.words[0x6e4u / 4u] == 0u &&
              no_path_state.words[0x6fcu / 4u] == 0x40200000u &&
              no_path.calls == std::vector<std::string>{"path"},
          "missing return path leaves mode two and clears target");

    auto rejected_state = initial;
    Runtime rejected;
    rejected.accepted = false;
    Check(pal4::MonsterPursuitReturnState::Enter(
              rejected_state, 0x5000u, 0.25f, rejected) ==
                  rejected.reset_result &&
              rejected_state.words[0x6e0u / 4u] == 0u &&
              rejected_state.words[0x6e4u / 4u] == 0u &&
              rejected.calls == std::vector<std::string>{
                  "path", "apply-path", "reset"},
          "rejected movement invokes pursuit reset");

    auto accepted_state = initial;
    Runtime accepted;
    accepted.speed = 12.5f;
    Check(pal4::MonsterPursuitReturnState::Enter(
              accepted_state, 0x5000u, 0.25f, accepted) ==
                  accepted.speed_result &&
              accepted_state.words[0x6e0u / 4u] == 2u &&
              accepted_state.words[0x6e4u / 4u] == 0u &&
              accepted.observed_manager == std::array<float, 3>{
                  1.0f, 2.0f, 3.0f} &&
              accepted.observed_target == std::array<float, 3>{
                  8.0f, 9.0f, 7.0f} &&
              accepted.observed_speed == 12.5f &&
              accepted.calls == std::vector<std::string>{
                  "path", "apply-path", "speed-config", "set-speed"},
          "return path applies saved position and configured speed");
}

}  // namespace

int main() {
    TargetAndPathGates();
    SuccessfulPathAndSpeed();
    DefaultStub();
    ReturnStateBranches();
    std::cout << "monster pursuit speed cases passed\n";
}
