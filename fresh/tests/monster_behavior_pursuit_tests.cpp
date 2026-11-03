#include "pal4/monster_behavior_manager.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterPursuitRuntime {
    std::vector<std::string> calls;
    pal4::ScriptEventProcessorRuntimeStub processor;
    std::string animation{"Z02"};
    bool proximity{};
    bool player_busy{};
    bool distance{};
    bool reached{};
    bool speed_result{};
    bool interaction{};
    std::uint32_t select_z_result{1u};
    std::uint32_t select_c_result{1u};
    std::uint32_t observed_target{};

    std::string_view AnimationName(std::uint32_t address) override {
        Check(address == 0x5170u, "animation subobject address");
        calls.emplace_back("animation-name");
        return animation;
    }
    std::uint32_t SelectAnimation(std::uint32_t address,
                                  const char* name) override {
        Check(address == 0x5170u, "animation selection address");
        calls.push_back(std::string("select:") + name);
        if (std::string_view(name) == "Z02" ||
            std::string_view(name) == "Z01") return select_z_result;
        return select_c_result;
    }
    bool CheckProximity(pal4::MonsterBehaviorManagerState&,
                        std::uint32_t address, float) override {
        Check(address == 0x5000u, "proximity manager address");
        calls.emplace_back("proximity");
        return proximity;
    }
    bool PlayerControlBusy() override {
        calls.emplace_back("player-busy");
        return player_busy;
    }
    bool CheckDistanceAndFlag(const pal4::MonsterBehaviorManagerState&,
                              std::uint32_t address, float) override {
        Check(address == 0x5000u, "distance manager address");
        calls.emplace_back("distance");
        return distance;
    }
    bool CheckTargetReached(const pal4::MonsterBehaviorManagerState&,
                            std::uint32_t address, float) override {
        Check(address == 0x5000u, "target manager address");
        calls.emplace_back("target");
        return reached;
    }
    void ProcessCollisionAndConfig(pal4::MonsterBehaviorManagerState&,
                                   std::uint32_t address, float) override {
        Check(address == 0x5000u, "collision manager address");
        calls.emplace_back("collision");
    }
    bool ProcessPursuitCollisionAndSpeed(
        const pal4::MonsterBehaviorManagerState&,
        std::uint32_t address, float,
        std::uint32_t target_word) override {
        Check(address == 0x5000u, "speed manager address");
        observed_target = target_word;
        calls.emplace_back("speed");
        return speed_result;
    }
    bool ProcessObjectInteraction(pal4::MonsterBehaviorManagerState&,
                                  std::uint32_t address,
                                  std::uint32_t target_word,
                                  float) override {
        Check(address == 0x5000u, "interaction manager address");
        observed_target = target_word;
        calls.emplace_back("interaction");
        return interaction;
    }
    void ProcessConfigAndEvents(std::uint32_t address, float) override {
        Check(address == 0x5000u, "config manager address");
        calls.emplace_back("config");
    }
    void InitializeCombatSystem(std::uint32_t address,
                                std::int32_t argument) override {
        Check(address == 0x5000u && argument == -1,
              "combat manager address and argument");
        calls.emplace_back("combat");
    }
    std::uint32_t ResetPursuit(pal4::MonsterBehaviorManagerState& state,
                      std::uint32_t address) override {
        Check(address == 0x5000u, "reset manager address");
        calls.emplace_back("reset");
        state.words[0x6e0u / 4u] = 0u;
        state.words[0x708u / 4u] = 0u;
        state.words[0x70cu / 4u] = 0u;
        return 0u;
    }
    pal4::IScriptEventProcessorRuntime& ProcessorRuntime() override {
        return processor;
    }
    void CleanupScriptObjectCollection(std::uint32_t address) override {
        Check(address == 0x5688u, "script collection address");
        calls.emplace_back("script-cleanup");
    }
    void LoadFuzzyBehaviorConfig(std::uint32_t address,
                                 std::uint32_t child) override {
        Check(address == 0x5000u && child == 0x7000u,
              "fuzzy config arguments");
        calls.emplace_back("fuzzy");
    }
    void DispatchComponentField(std::uint32_t address,
                                std::uint32_t slot,
                                std::uint32_t field,
                                std::uint32_t argument) override {
        Check(address == 0x5000u && argument == 0u &&
                  ((slot == 4u && field == 0x8018u) ||
                   (slot == 8u && field == 0x8024u)),
              "reset component dispatch arguments");
        calls.emplace_back(slot == 4u ? "dispatch4" : "dispatch8");
    }
    void SetRecordErrorFlag(std::uint32_t address,
                            std::uint8_t flag) override {
        Check(flag == 1u, "record flag value");
        calls.push_back("record:" + std::to_string(address));
    }
    void ProcessActionWithScript(std::uint32_t address, float) override {
        Check(address == 0x5000u, "script action address");
        calls.emplace_back("action");
    }
    void VisitArenaObjects(std::uint32_t address) override {
        Check(address == 0x5000u, "arena visit address");
        calls.emplace_back("visit");
    }
};

using State = pal4::MonsterBehaviorManagerState;
using Manager = pal4::MonsterBehaviorManager;

void SetFloat(State& state, std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}
float GetFloat(const State& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

void IdleGatesAndTransition() {
    for (const auto [nearby, stopped, busy] :
         {std::tuple{false, false, false},
          std::tuple{true, true, false},
          std::tuple{true, false, true}}) {
        State state{};
        Runtime runtime;
        runtime.proximity = nearby;
        runtime.player_busy = busy;
        state.words[0x704u / 4u] = stopped ? 1u : 0u;
        SetFloat(state, 0x708u, 4.0f);
        SetFloat(state, 0x70cu, 5.0f);
        Check(!Manager::UpdatePursuitState(
                  state, 0x5000u, 0.5f, runtime) &&
                  state.words[0x6e0u / 4u] == 0u &&
                  state.words[0x708u / 4u] == 0u &&
                  state.words[0x70cu / 4u] == 0u &&
                  runtime.calls.back() == "visit",
              "idle gate keeps mode zero and clears both timers");
    }

    State state{};
    Runtime runtime;
    runtime.proximity = true;
    state.words[0x714u / 4u] = 0x8800u;
    state.words[0x718u / 4u] = 0x8810u;
    state.words[0x724u / 4u] = 0x9000u;
    state.words[0x728u / 4u] = 0x9020u;
    SetFloat(state, 0x6f8u, 3.0f);
    Check(Manager::UpdatePursuitState(
              state, 0x5000u, 0.5f, runtime) &&
              state.words[0x6e0u / 4u] == 1u &&
              GetFloat(state, 0x6fcu) == 3.0f &&
              state.words[0x718u / 4u] == 0x8800u &&
              state.words[0x728u / 4u] == 0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "proximity", "player-busy", "record:36864",
                  "record:36880", "action", "script-cleanup", "visit"},
          "proximity enters pursuit and resets both collection cursors");
}

void PursuitCollisionAndInteraction() {
    for (const auto gate : {0u, 1u, 2u}) {
        State state{};
        Runtime runtime;
        state.words[0x6e0u / 4u] = 1u;
        state.words[0x5f4u / 4u] = gate == 0u ? 3u : 0u;
        state.words[0x704u / 4u] = gate == 1u ? 1u : 0u;
        runtime.distance = gate == 2u;
        Check(Manager::UpdatePursuitState(
                  state, 0x5000u, 0.5f, runtime) &&
                  GetFloat(state, 0x708u) == 0.5f &&
                  runtime.calls.back() == "visit" &&
                  runtime.calls[runtime.calls.size() - 2u] == "collision",
              "behavior, stop flag and distance trigger collision path");
    }

    State state{};
    Runtime runtime;
    state.words[0x6e0u / 4u] = 1u;
    state.words[0x6e4u / 4u] = 0x9900u;
    SetFloat(state, 0x6f8u, 2.0f);
    SetFloat(state, 0x6fcu, 1.0f);
    runtime.animation = "z02";
    Check(Manager::UpdatePursuitState(
              state, 0x5000u, 0.5f, runtime) &&
              GetFloat(state, 0x6fcu) == 1.5f &&
              runtime.observed_target == 0x9900u &&
              runtime.calls == std::vector<std::string>{
                  "animation-name", "distance", "interaction", "visit"},
          "below target accumulates timer and accepts case-insensitive name");

    state.words[0x6e0u / 4u] = 1u;
    SetFloat(state, 0x6fcu, 2.0f);
    runtime.calls.clear();
    runtime.interaction = true;
    Check(Manager::UpdatePursuitState(
              state, 0x5000u, 0.5f, runtime) &&
              state.words[0x6fcu / 4u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "animation-name", "distance", "speed", "collision",
                  "interaction", "config", "combat", "visit"},
          "at target resets timer and proceeds through interaction to combat");

    State unordered{};
    Runtime nan_runtime;
    unordered.words[0x6e0u / 4u] = 1u;
    SetFloat(unordered, 0x6f8u, 2.0f);
    SetFloat(unordered, 0x6fcu, std::nanf(""));
    Check(Manager::UpdatePursuitState(
              unordered, 0x5000u, 0.5f, nan_runtime) &&
              std::isnan(GetFloat(unordered, 0x6fcu)) &&
              nan_runtime.calls == std::vector<std::string>{
                  "animation-name", "distance", "interaction", "visit"},
          "unordered pursuit timer stays on accumulation branch");
}

void ReturnThresholdAndTarget() {
    State state{};
    Runtime runtime;
    state.words[0x6e0u / 4u] = 2u;
    SetFloat(state, 0x708u, 1.0f);
    SetFloat(state, 0x70cu, 2.5f);
    runtime.animation = "C03";
    Check(Manager::UpdatePursuitState(
              state, 0x5000u, 0.5f, runtime) &&
              GetFloat(state, 0x70cu) == 3.0f &&
              runtime.calls == std::vector<std::string>{
                  "animation-name", "animation-name", "target", "visit"},
          "equal triple threshold stays in return mode");
    runtime.calls.clear();
    Check(!Manager::UpdatePursuitState(
              state, 0x5000u, 0.5f, runtime) &&
              runtime.calls == std::vector<std::string>{"reset", "visit"},
          "greater-than threshold invokes reset before arena traversal");

    State reached{};
    Runtime finish;
    reached.words[0x6e0u / 4u] = 2u;
    reached.words[0x654u / 4u] = 1u;
    reached.words[0x6d0u / 4u] = 0x7000u;
    SetFloat(reached, 0x708u, 10.0f);
    finish.reached = true;
    finish.animation = "Z02";
    finish.select_c_result = 0u;
    Check(!Manager::UpdatePursuitState(
              reached, 0x5000u, 0.5f, finish) &&
              reached.words[0x6e0u / 4u] == 0u &&
              reached.words[0x708u / 4u] == 0u &&
              reached.words[0x70cu / 4u] == 0u &&
              (reached.words[(0x658u + 24u) / 4u] & 0xffu) == 1u &&
              finish.calls == std::vector<std::string>{
                  "animation-name", "target", "script-cleanup", "fuzzy",
                  "animation-name", "animation-name", "select:C01",
                  "select:Z01", "visit"},
          "reached target reloads fuzzy behavior and falls back to Z01");

    State unordered{};
    Runtime nan_runtime;
    unordered.words[0x6e0u / 4u] = 2u;
    SetFloat(unordered, 0x708u, std::nanf(""));
    Check(Manager::UpdatePursuitState(
              unordered, 0x5000u, 0.5f, nan_runtime) &&
              nan_runtime.calls.front() == "animation-name",
          "unordered threshold follows the non-reset x87 branch");

    for (const auto behavior : {2u, 3u}) {
        State special{};
        Runtime special_runtime;
        special.words[0x6e0u / 4u] = 2u;
        special.words[0x5f4u / 4u] = behavior;
        SetFloat(special, 0x708u, 10.0f);
        Check(!Manager::UpdatePursuitState(
                  special, 0x5000u, 0.5f, special_runtime) &&
                  special.words[0x6e0u / 4u] == 0u &&
                  special.words[0x708u / 4u] == 0u &&
                  special.words[0x70cu / 4u] == 0u &&
                  special_runtime.calls == std::vector<std::string>{
                      "animation-name", "reset", "visit"},
              "return behavior 2 and 3 reset before arena traversal");
    }
}

void OtherModeAndStub() {
    State state{};
    Runtime runtime;
    state.words[0x6e0u / 4u] = 7u;
    Check(Manager::UpdatePursuitState(
              state, 0x5000u, 0.5f, runtime) &&
              runtime.calls == std::vector<std::string>{"visit"},
          "unknown nonzero mode still visits arena and returns true");
    pal4::MonsterPursuitRuntimeStub stub;
    State empty{};
    Check(!Manager::UpdatePursuitState(
              empty, 0x5000u, 0.5f, stub),
          "external pursuit services have a linkable stub");
}

void ResetPursuitBehavior() {
    for (const auto [source, child] :
         {std::pair{0u, 0u}, std::pair{0x8000u, 0u}}) {
        State state{};
        Runtime runtime;
        state.words[0x6e0u / 4u] = 2u;
        state.words[0x654u / 4u] = source;
        state.words[0x6d0u / 4u] = child;
        SetFloat(state, 0x708u, 1.0f);
        SetFloat(state, 0x70cu, 2.0f);
        Check(pal4::MonsterPursuitReset::Reset(
                  state, 0x5000u, runtime) == 0u &&
                  state.words[0x6e0u / 4u] == 0u &&
                  state.words[0x708u / 4u] == 0u &&
                  state.words[0x70cu / 4u] == 0u &&
                  runtime.calls.empty(),
              "missing source or child skips cleanup but clears state");
    }

    State state{};
    Runtime runtime;
    state.words[0x6e0u / 4u] = 2u;
    state.words[0x654u / 4u] = 0x8000u;
    state.words[0x6d0u / 4u] = 0x7000u;
    runtime.animation = "Z02";
    runtime.select_c_result = 0u;
    runtime.select_z_result = 17u;
    Check(pal4::MonsterPursuitReset::Reset(
              state, 0x5000u, runtime) == 17u &&
              state.words[0x6e0u / 4u] == 0u &&
              (state.words[(0x658u + 24u) / 4u] & 0xffu) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "script-cleanup", "fuzzy", "dispatch4", "dispatch8",
                  "animation-name", "animation-name",
                  "select:C01", "select:Z01"},
          "reset reloads behavior and falls back to Z01");

    State selected{};
    Runtime first;
    selected.words[0x654u / 4u] = 0x8000u;
    selected.words[0x6d0u / 4u] = 0x7000u;
    first.select_c_result = 23u;
    Check(pal4::MonsterPursuitReset::Reset(
              selected, 0x5000u, first) == 23u &&
              first.calls.back() == "select:C01",
          "successful C01 selection returns its result");

    State default_state{};
    default_state.words[0x654u / 4u] = 0x8000u;
    default_state.words[0x6d0u / 4u] = 0x7000u;
    default_state.words[0x6e0u / 4u] = 2u;
    pal4::MonsterPursuitRuntimeStub stub;
    Check(stub.ResetPursuit(default_state, 0x5000u) == 0u &&
              default_state.words[0x6e0u / 4u] == 0u &&
              (default_state.words[(0x658u + 24u) / 4u] & 0xffu) == 1u,
          "default runtime composes reset and fuzzy loader stubs");
}

}  // namespace

int main() {
    IdleGatesAndTransition();
    PursuitCollisionAndInteraction();
    ReturnThresholdAndTarget();
    OtherModeAndStub();
    ResetPursuitBehavior();
}
