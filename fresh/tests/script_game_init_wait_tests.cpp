#include "pal4/script_game_init_wait.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptGameInitRuntime {
public:
    pal4::GameInitWaitState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    std::uint32_t current_state = 0x7000u;
    std::uint32_t state_identity = 1u;

    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x4000u;
    }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t first,
                              std::uint32_t second) override {
        Check(game == 0x4000u && first == 0u && second == 0u,
              "game init operands");
        calls.emplace_back("initialize");
    }
    pal4::GameInitWaitState* AllocateWait(std::uint32_t bytes) override {
        Check(bytes == 0x24u, "game init wait size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::GameInitWaitState& handler,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && mode == 1u && queue == 0x6054u,
              "game init wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::GameInitWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x84969cu &&
                  handler.state_identity == 0u,
              "game init wait state before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(std::uint32_t queue,
                             pal4::GameInitWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "game init handler queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
    std::uint32_t CurrentState(std::uint32_t game) override {
        Check(game == 0x4000u, "current state game");
        calls.emplace_back("current state");
        return current_state;
    }
    std::uint32_t StateIdentity(std::uint32_t current) override {
        Check(current == 0x7000u, "state identity receiver");
        calls.emplace_back("state identity");
        return state_identity;
    }
    void CloseWait(pal4::GameInitWaitState& handler) override {
        Check(&handler == &state, "close wait receiver");
        calls.emplace_back("close");
    }
    std::uint32_t DestroyWait(
        pal4::GameInitWaitState& handler) override {
        Check(&handler == &state, "destroy wait receiver");
        calls.emplace_back("destroy");
        return 0x8000u;
    }
};

void CreationCases() {
    Runtime runtime;
    pal4::ScriptGameInitWait wait(runtime);
    Check(wait.Start() == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "game", "initialize", "allocate", "executor",
                  "construct", "flush", "executor", "add handler"},
          "game init queues blocking state after initialization");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptGameInitWait failed_wait(failed);
    Check(failed_wait.Start() == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "game", "initialize", "allocate", "executor",
                  "add handler"},
          "game init allocation failure still queues null");
}

void CallbackCases() {
    Runtime runtime;
    pal4::ScriptGameInitWait wait(runtime);
    runtime.state.state_identity = 9u;
    wait.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "game", "current state", "state identity"},
          "different state identity keeps handler open");

    runtime.calls.clear();
    runtime.state_identity = 9u;
    wait.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "game", "current state", "state identity", "close"},
          "matching state identity closes handler");

    runtime.calls.clear();
    runtime.current_state = 0u;
    wait.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "game", "current state", "close"},
          "absent state closes without virtual identity call");

    runtime.calls.clear();
    wait.Close(runtime.state);
    Check(wait.Destroy(runtime.state) == 0x8000u &&
              runtime.calls == std::vector<std::string>{"close", "destroy"},
          "vtable forwarding thunks preserve close and destructor results");
}

}  // namespace

int main() {
    try {
        CreationCases();
        CallbackCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
