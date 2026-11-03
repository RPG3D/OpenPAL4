#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_proximity_checker.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
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

struct Runtime final : pal4::IMonsterProximityRuntime {
    pal4::MonsterProximityActor actor{};
    double distance{};
    float radius{5.0f};
    std::uint32_t clump{0x9000u};
    bool blocked{};
    float observed_dx{};
    float observed_dz{};
    std::uint32_t observed_config{};
    std::array<float, 6> observed_endpoints{};
    std::uint32_t observed_manager{};
    std::uint32_t observed_position_address{};
    std::vector<std::string> calls;

    pal4::MonsterProximityActor ControlledActor() override {
        calls.emplace_back("actor");
        return actor;
    }
    double PlanarDistance(float dx, float dz) override {
        calls.emplace_back("distance");
        observed_dx = dx;
        observed_dz = dz;
        return distance;
    }
    float DetectionRadius(std::uint32_t config) override {
        calls.emplace_back("radius");
        observed_config = config;
        return radius;
    }
    std::uint32_t CollisionClump() override {
        calls.emplace_back("clump");
        return clump;
    }
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t address) override {
        Check(address == clump, "collision receives movement clump");
        calls.emplace_back("collision");
        observed_endpoints = endpoints;
        return blocked;
    }
    void NotifyReturnTargetReached(std::uint32_t manager,
                                   std::uint32_t position_address,
                                   std::uint32_t argument) override {
        Check(argument == 0u, "return callback third argument");
        calls.emplace_back("return-callback");
        observed_manager = manager;
        observed_position_address = position_address;
    }
};

State InitialState() {
    State state{};
    state.words[0x654u / 4u] = 0x8000u;
    state.words[0x6e4u / 4u] = 0xdeadbeefu;
    SetFloat(state, 0xacu, 1.0f);
    SetFloat(state, 0xb0u, 2.0f);
    SetFloat(state, 0xb4u, 3.0f);
    SetFloat(state, 0xe0u, 4.0f);
    SetFloat(state, 0xe4u, 5.0f);
    SetFloat(state, 0xe8u, 6.0f);
    return state;
}

Runtime InitialRuntime() {
    Runtime runtime;
    runtime.actor.address = 0x7000u;
    runtime.actor.active = true;
    runtime.actor.position = {4.0f, 4.0f, 7.0f};
    runtime.actor.collision_position = {8.0f, 9.0f, 10.0f};
    runtime.actor.half_height = 1.5f;
    runtime.distance = 5.0;
    return runtime;
}

void MissingAndInactiveActor() {
    for (const auto mode : {0u, 1u}) {
        auto state = InitialState();
        auto runtime = InitialRuntime();
        if (mode == 0u) runtime.actor.address = 0u;
        else runtime.actor.active = false;
        Check(!pal4::MonsterProximityChecker::Check(state, runtime) &&
                  state.words[0x6e4u / 4u] == 0u &&
                  runtime.calls == std::vector<std::string>{"actor"},
              "missing or inactive actor clears target and returns early");
    }
}

void DistanceAndVerticalGates() {
    for (const auto distance : {5.01, std::nan("")}) {
        auto state = InitialState();
        auto runtime = InitialRuntime();
        runtime.distance = distance;
        Check(!pal4::MonsterProximityChecker::Check(state, runtime) &&
                  runtime.calls == std::vector<std::string>{
                      "actor", "distance", "radius"} &&
                  runtime.observed_dx == 3.0f &&
                  runtime.observed_dz == 4.0f &&
                  runtime.observed_config == 0x8000u,
              "outside or unordered planar radius skips vertical gate");
    }
    for (const auto half_height : {1.0f, std::nanf("")}) {
        auto state = InitialState();
        auto runtime = InitialRuntime();
        runtime.actor.half_height = half_height;
        runtime.distance = runtime.radius;
        Check(!pal4::MonsterProximityChecker::Check(state, runtime) &&
                  runtime.calls == std::vector<std::string>{
                      "actor", "distance", "radius"},
              "equal or unordered vertical reach skips collision");
    }
}

void CollisionAndSnapshot() {
    auto state = InitialState();
    auto runtime = InitialRuntime();
    runtime.blocked = true;
    Check(!pal4::MonsterProximityChecker::Check(state, runtime) &&
              state.words[0x6e4u / 4u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "actor", "distance", "radius", "clump", "collision"},
          "blocked collision rejects target");
    runtime.calls.clear();
    runtime.blocked = false;
    Check(pal4::MonsterProximityChecker::Check(state, runtime) &&
              state.words[0x6e4u / 4u] == 0x7000u &&
              runtime.observed_endpoints == std::array<float, 6>{
                  4.0f, 5.0f, 6.0f, 8.0f, 9.0f, 10.0f} &&
              state.words[0x6e8u / 4u] == state.words[0xacu / 4u] &&
              state.words[0x6ecu / 4u] == state.words[0xb0u / 4u] &&
              state.words[0x6f0u / 4u] == state.words[0xb4u / 4u],
          "clear collision stores actor and exact manager position words");
}

void DefaultRuntimeComposition() {
    State state{};
    state.words[0x1f0u / 4u] = 1u;
    state.words[0x6e0u / 4u] = 1u;
    pal4::MonsterBehaviorManagerRuntimeStub runtime;
    (void)pal4::MonsterBehaviorManager::Update(
        state, 0x5000u, 0.5f, 0u, runtime);
    Check(std::bit_cast<float>(state.words[0x708u / 4u]) == 0.5f,
          "default manager runtime composes pursuit update");

    state.words[0x6e0u / 4u] = 2u;
    SetFloat(state, 0x708u, 0.0f);
    SetFloat(state, 0x70cu, 1.0f);
    Check(runtime.ProcessManagerState(state, 0x5000u, 0.5f) == 0u &&
              state.words[0x6e0u / 4u] == 0u,
          "default runtime returns pursuit reset result");
}

void SavedPositionThreshold() {
    auto state = InitialState();
    SetFloat(state, 0x6e8u, 10.0f);
    SetFloat(state, 0x6f0u, 30.0f);
    for (const auto [distance, expected] :
         {std::pair{999.0, false}, std::pair{1000.0, true},
          std::pair{1001.0, true}, std::pair{std::nan(""), true}}) {
        auto runtime = InitialRuntime();
        runtime.distance = distance;
        Check(pal4::MonsterProximityChecker::BeyondReturnThreshold(
                  state, runtime) == expected &&
                  runtime.observed_dx == 9.0f &&
                  runtime.observed_dz == 27.0f &&
                  runtime.calls == std::vector<std::string>{"distance"},
              "saved position distance uses strict lower threshold");
    }
}

void ReturnTargetReached() {
    auto state = InitialState();
    SetFloat(state, 0x6e8u, 10.0f);
    SetFloat(state, 0x6f0u, 30.0f);
    for (const auto distance : {0.05001, std::nan("")}) {
        auto runtime = InitialRuntime();
        runtime.distance = distance;
        Check(!pal4::MonsterProximityChecker::CheckReturnTargetReached(
                  state, 0x5000u, runtime) &&
                  runtime.calls == std::vector<std::string>{"distance"},
              "outside or unordered return distance skips callback");
    }
    for (const auto distance : {0.05, 0.0}) {
        auto runtime = InitialRuntime();
        runtime.distance = distance;
        Check(pal4::MonsterProximityChecker::CheckReturnTargetReached(
                  state, 0x5000u, runtime) &&
                  runtime.observed_dx == 9.0f &&
                  runtime.observed_dz == 27.0f &&
                  runtime.observed_manager == 0x5000u &&
                  runtime.observed_position_address == 0x56e8u &&
                  runtime.calls == std::vector<std::string>{
                      "distance", "return-callback"},
              "at or within return target invokes virtual slot four");
    }
}

}  // namespace

int main() {
    MissingAndInactiveActor();
    DistanceAndVerticalGates();
    CollisionAndSnapshot();
    SavedPositionThreshold();
    ReturnTargetReached();
    DefaultRuntimeComposition();
    std::cout << "monster proximity cases passed\n";
}
