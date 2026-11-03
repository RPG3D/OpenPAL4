#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_interaction_processor.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
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

using State = pal4::MonsterBehaviorManagerState;

void SetFloat(State& state, std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

State InitialState() {
    State state{};
    state.words[0x654u / 4u] = 0x8000u;
    state.words[0x700u / 4u] = 9u;
    for (std::uint32_t i = 0u; i != 4u; ++i)
        state.words[(0xd0u + 4u * i) / 4u] = 0x11110000u + i;
    SetFloat(state, 0xacu, 1.0f);
    SetFloat(state, 0xb0u, 2.0f);
    SetFloat(state, 0xb4u, 3.0f);
    SetFloat(state, 0xdcu, 1.0f);
    return state;
}

struct Runtime final : pal4::IMonsterInteractionRuntime {
    std::vector<std::string> calls;
    bool busy{};
    pal4::MonsterInteractionTarget target{{4.0f, 2.0f, 7.0f}};
    double distance{5.0};
    float distance_base{4.0f};
    std::array<float, 3> target_forward{1.0f, 0.0f, 0.0f};
    std::array<float, 3> manager_forward{1.0f, 0.0f, 0.0f};
    double angle{};
    std::string script_name;
    std::string path{"Scene/Script"};
    bool cached{};
    std::uint32_t interpreter{0x9000u};
    bool executed{true};
    std::int32_t pending{1};
    float observed_dx{};
    float observed_dz{};
    float observed_forward_dot{};
    std::array<float, 3> normalized_direction{};

    bool PlayerControlBusy() override {
        calls.emplace_back("busy");
        return busy;
    }
    pal4::MonsterInteractionTarget Target(std::uint32_t address) override {
        Check(address == 0x7000u, "target object address");
        calls.emplace_back("target");
        return target;
    }
    double PlanarDistance(float dx, float dz) override {
        calls.emplace_back("distance");
        observed_dx = dx;
        observed_dz = dz;
        return distance;
    }
    float PlayerDistanceBase() override {
        calls.emplace_back("base");
        return distance_base;
    }
    std::array<float, 3> TargetForward(std::uint32_t address) override {
        Check(address == 0x7000u, "target transform owner");
        calls.emplace_back("target-forward");
        return target_forward;
    }
    std::array<float, 3> ManagerForward(std::uint32_t address) override {
        Check(address == 0x5000u, "manager transform owner");
        calls.emplace_back("manager-forward");
        return manager_forward;
    }
    void NormalizeDirection(std::array<float, 3>& direction) override {
        calls.emplace_back("normalize-direction");
        const auto length = std::sqrt(direction[0] * direction[0] +
                                      direction[1] * direction[1] +
                                      direction[2] * direction[2]);
        if (length != 0.0)
            for (auto& component : direction)
                component = static_cast<float>(component / length);
        normalized_direction = direction;
    }
    double AngleForForwardDot(float dot) override {
        calls.emplace_back("angle");
        observed_forward_dot = dot;
        return angle;
    }
    std::string ReadScriptName(std::uint32_t source,
        const std::array<std::uint32_t, 4>& key,
        const char* attribute) override {
        Check(source == 0x8000u &&
                  key == std::array<std::uint32_t, 4>{
                      0x11110000u, 0x11110001u, 0x11110002u, 0x3f800000u} &&
                  std::string(attribute) == "AAB_MST_ESSBEH_script_func",
              "XML query uses source and four-word combat key");
        calls.emplace_back("read-script");
        return script_name;
    }
    void InitializeScripts() override { calls.emplace_back("script-init"); }
    std::string ArenaScriptPath() override {
        calls.emplace_back("script-path");
        return path;
    }
    bool ScriptCached(const std::string& full_path) override {
        Check(full_path == "Scene/Script[battle]", "bracketed script path");
        calls.emplace_back("cache");
        return cached;
    }
    std::uint32_t CreateInterpreter(const std::string& script_path,
        const std::string& full_path,
        const std::string& name) override {
        Check(script_path == path && full_path == "Scene/Script[battle]" &&
                  name == "battle", "interpreter arguments");
        calls.emplace_back("create");
        return interpreter;
    }
    bool ExecuteInterpreter(std::uint32_t address) override {
        Check(address == interpreter, "execute interpreter address");
        calls.emplace_back("execute");
        return executed;
    }
    void HandleInterpreterError(std::uint32_t address) override {
        Check(address == interpreter, "error interpreter address");
        calls.emplace_back("error");
    }
    void ReleaseInterpreter(std::uint32_t address) override {
        Check(address == interpreter, "release interpreter address");
        calls.emplace_back("release");
    }
    void ShowMissingScriptError() override {
        calls.emplace_back("missing-dialog");
    }
    void CleanupScripts() override { calls.emplace_back("cleanup"); }
    std::int32_t PendingStateCount(std::uint32_t source) override {
        Check(source == 0x8000u, "pending-state source address");
        calls.emplace_back("pending");
        return pending;
    }
    void ReactivateManager(std::uint32_t address,
                           std::uint32_t flag) override {
        Check(address == 0x5000u && flag == 0x40000000u,
              "arena reactivation arguments");
        calls.emplace_back("reactivate");
    }
    void RestoreRenderCallback(std::uint32_t address) override {
        Check(address == 0x5000u, "render callback manager address");
        calls.emplace_back("render-callback");
    }
};

bool Process(State& state, Runtime& runtime, std::uint32_t target = 0x7000u) {
    return pal4::MonsterInteractionProcessor::Process(
        state, 0x5000u, target, runtime);
}

void EarlyGates() {
    auto state = InitialState();
    Runtime runtime;
    Check(!Process(state, runtime, 0u) && runtime.calls.empty() &&
              state.words[0x700u / 4u] == 9u,
          "null target exits before actor control access");
    runtime.busy = true;
    Check(!Process(state, runtime) &&
              runtime.calls == std::vector<std::string>{"busy"} &&
              state.words[0x700u / 4u] == 9u,
          "busy actor control exits without distance query");
    for (const auto distance : {5.01, std::numeric_limits<double>::quiet_NaN()}) {
        auto far_state = InitialState();
        Runtime far;
        far.distance = distance;
        Check(!Process(far_state, far) &&
                  far.observed_dx == 3.0f && far.observed_dz == 4.0f &&
                  far.calls == std::vector<std::string>{
                      "busy", "target", "distance", "base"} &&
                  far_state.words[0x700u / 4u] == 9u,
              "far or unordered target exits before facing reset");
    }
}

void FacingClassification() {
    for (const auto [degrees, expected] :
         {std::pair{0.0, 1u}, std::pair{44.0, 1u},
          std::pair{46.0, 0u}, std::pair{314.0, 0u},
          std::pair{316.0, 1u}}) {
        auto state = InitialState();
        Runtime runtime;
        runtime.angle = degrees / static_cast<double>(57.29578f);
        Check(Process(state, runtime) &&
                  state.words[0x700u / 4u] == expected &&
                  runtime.observed_forward_dot == 1.0f &&
                  runtime.normalized_direction[0] == 0.6f,
              "facing state follows open 45/315 degree window");
    }
    auto away_state = InitialState();
    Runtime away;
    away.manager_forward = {-1.0f, 0.0f, 0.0f};
    away.target_forward = {-1.0f, 0.0f, 0.0f};
    Check(Process(away_state, away) &&
              away_state.words[0x700u / 4u] == 2u,
          "same-facing angle with negative target dot records away state");

    auto reverse_state = InitialState();
    Runtime reverse;
    reverse.manager_forward = {0.0f, 0.0f, 1.0f};
    reverse.target_forward = {1.0f, 0.0f, 0.0f};
    reverse.angle = 10.0 / static_cast<double>(57.29578f);
    Check(Process(reverse_state, reverse) &&
              reverse_state.words[0x700u / 4u] == 1u,
          "forward X comparison reflects angle through full circle");
}

void ScriptPaths() {
    auto state = InitialState();
    Runtime empty;
    Check(Process(state, empty) &&
              empty.calls.back() == "read-script" &&
              state.words[0x700u / 4u] == 1u,
          "empty script name returns true after facing classification");

    for (const auto [cached, interpreter, executed, suffix] :
         {std::tuple{true, 0x9000u, true,
                     std::vector<std::string>{"cache", "cleanup", "pending"}},
          std::tuple{false, 0u, true,
                     std::vector<std::string>{"cache", "create",
                         "missing-dialog", "cleanup", "pending"}},
          std::tuple{false, 0x9000u, false,
                     std::vector<std::string>{"cache", "create", "execute",
                         "missing-dialog", "cleanup", "pending"}},
          std::tuple{false, 0x9000u, true,
                     std::vector<std::string>{"cache", "create", "execute",
                         "error", "release", "cleanup", "pending"}}}) {
        auto script_state = InitialState();
        Runtime runtime;
        runtime.script_name = "battle";
        runtime.cached = cached;
        runtime.interpreter = interpreter;
        runtime.executed = executed;
        Check(Process(script_state, runtime) &&
                  std::equal(suffix.begin(), suffix.end(),
                             runtime.calls.end() - suffix.size()),
              "cache, interpreter and error paths preserve cleanup order");
    }

    auto reactivate_state = InitialState();
    Runtime reactivate;
    reactivate.script_name = "battle";
    reactivate.cached = true;
    reactivate.pending = 0;
    Check(!Process(reactivate_state, reactivate) &&
              reactivate.calls[reactivate.calls.size() - 3u] == "pending" &&
              reactivate.calls[reactivate.calls.size() - 2u] == "reactivate" &&
              reactivate.calls.back() == "render-callback",
          "no pending state reactivates manager and restores render callback");
}

}  // namespace

int main() {
    EarlyGates();
    FacingClassification();
    ScriptPaths();
    std::cout << "monster interaction cases passed\n";
}
