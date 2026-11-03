#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_pursuit_collision_gate.h"

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

struct Runtime final : pal4::IMonsterPursuitCollisionGateRuntime {
    bool pursuit_result{true};
    bool player_flag{};
    std::uint32_t player{0x9000u};
    std::uint32_t return_result{0xa000u};
    std::vector<std::string> calls;

    bool ProcessPursuit(const pal4::MonsterBehaviorManagerState& state,
                        std::uint32_t address, float elapsed,
                        std::uint32_t target) override {
        Check(address == 0x5000u && elapsed == 1.25f &&
                  target == state.words[0x6e4u / 4u],
              "pursuit call arguments");
        calls.emplace_back("pursuit");
        return pursuit_result;
    }
    std::uint32_t EnterReturn(pal4::MonsterBehaviorManagerState&,
                              std::uint32_t address,
                              float elapsed) override {
        Check(address == 0x5000u && elapsed == 1.25f,
              "return-state call arguments");
        calls.emplace_back("return");
        return return_result;
    }
    std::uint32_t PlayerControlObject() override {
        calls.emplace_back("player");
        return player;
    }
    bool PlayerControlFlag(std::uint32_t address) override {
        Check(address == player, "player control flag object");
        calls.emplace_back("flag");
        return player_flag;
    }
};

pal4::MonsterBehaviorManagerState Manager() {
    pal4::MonsterBehaviorManagerState state{};
    state.words[0x1f0u / 4u] = 0x7777u;
    state.words[0x730u / 4u] = 0u;
    state.words[0x6e0u / 4u] = 1u;
    state.words[0x6e4u / 4u] = 0x8000u;
    return state;
}

std::uint32_t Process(pal4::MonsterBehaviorManagerState& state,
                      Runtime& runtime) {
    return pal4::MonsterPursuitCollisionGate::Process(
        state, 0x5000u, 1.25f, runtime);
}

void EarlyGatesAndRawReturns() {
    auto state = Manager();
    Runtime runtime;
    state.words[0x1f0u / 4u] = 0u;
    Check(Process(state, runtime) == 0u && runtime.calls.empty(),
          "inactive manager returns zero without callbacks");
    state.words[0x1f0u / 4u] = 0x7777u;
    state.words[0x730u / 4u] = 1u;
    Check(Process(state, runtime) == 0x7777u && runtime.calls.empty(),
          "state flag one returns initial manager field");
    state.words[0x730u / 4u] = 0u;
    for (const auto [mode, expected] : {
             std::pair{0u, 0xfffffffeu},
             std::pair{3u, 1u},
             std::pair{9u, 7u}}) {
        state.words[0x6e0u / 4u] = mode;
        Check(Process(state, runtime) == expected && runtime.calls.empty(),
              "other pursuit modes return the post-decrement EAX value");
    }
}

void ReturnStatePaths() {
    auto state = Manager();
    Runtime runtime;
    state.words[0x6e0u / 4u] = 2u;
    Check(Process(state, runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{"return"},
          "state two directly enters return mode");

    state = Manager();
    runtime.calls.clear();
    state.words[0x6e4u / 4u] = 0u;
    Check(Process(state, runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{"return"},
          "missing pursuit target directly enters return mode");

    state = Manager();
    runtime.calls.clear();
    runtime.pursuit_result = false;
    Check(Process(state, runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{
                  "pursuit", "return"},
          "failed pursuit enters return mode");

    runtime.calls.clear();
    runtime.pursuit_result = true;
    runtime.player_flag = true;
    Check(Process(state, runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{
                  "pursuit", "player", "flag", "return"},
          "player-control flag enters return mode after pursuit");
}

void SuccessfulPursuitAndStub() {
    auto state = Manager();
    Runtime runtime;
    Check(Process(state, runtime) == 0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "pursuit", "player", "flag"} &&
              state.words[0x6e0u / 4u] == 1u,
          "successful pursuit returns player-control object pointer");

    pal4::MonsterPursuitCollisionGateRuntimeStub stub;
    state.words[0x6e0u / 4u] = 2u;
    (void)pal4::MonsterPursuitCollisionGate::Process(
        state, 0x5000u, 1.25f, stub);
    Check(state.words[0x6e0u / 4u] == 2u &&
              state.words[0x6e4u / 4u] == 0u,
          "default boundary composes reconstructed return-state class");
}

}  // namespace

int main() {
    EarlyGatesAndRawReturns();
    ReturnStatePaths();
    SuccessfulPursuitAndStub();
    std::cout << "monster pursuit collision gate cases passed\n";
}
