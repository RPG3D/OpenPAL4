#include "pal4/script_current_player_timed_move.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptCurrentPlayerTimedMoveRuntime {
public:
    pal4::CurrentPlayerTimedMoveWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t player = 0x5000u;
    bool movement = true;
    bool allocation_fails = false;
    std::string animation = "walk";
    pal4::TimedMovePosition initial{1.0f, 2.0f, 3.0f};
    pal4::TimedMovePosition written{};

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current player manager");
        calls.emplace_back("current");
        return player;
    }
    bool HasMovementComponent(std::uint32_t handle) override {
        Check(handle == player, "movement component receiver");
        calls.emplace_back("movement");
        return movement;
    }
    std::string_view AnimationName(std::uint32_t handle) override {
        Check(handle == player, "animation receiver");
        calls.emplace_back("animation name");
        return animation;
    }
    void SelectAnimation(std::uint32_t handle,
                         std::string_view name) override {
        Check(handle == player && name == "C01", "selected animation");
        calls.emplace_back("select C01");
    }
    void SetAnimationFlag(std::uint32_t handle,
                          std::uint32_t flag) override {
        Check(handle == player && flag == 1u, "movement animation flag");
        calls.emplace_back("flag 1");
    }
    pal4::TimedMovePosition ReadPosition(
        std::uint32_t handle) override {
        Check(handle == player, "player position receiver");
        calls.emplace_back("read position");
        return initial;
    }
    pal4::CurrentPlayerTimedMoveWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x44u, "player timed move wait size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::CurrentPlayerTimedMoveWaitState& handler,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && mode == 7u && queue == 0x6054u,
              "player timed move wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::CurrentPlayerTimedMoveWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x849684u &&
                  handler.object == player &&
                  handler.target == pal4::TimedMovePosition{5, 6, 7} &&
                  handler.start == initial &&
                  handler.duration == 2.0f && handler.elapsed == 0.0f,
              "player timed move state before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue,
        pal4::CurrentPlayerTimedMoveWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "player timed move queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
    float Interpolate(float start, float end,
                       float ratio) override {
        Check(ratio == 0.25f || ratio == 1.0f,
              "player timed move interpolation ratio");
        calls.emplace_back("interpolate");
        return start + (end - start) * ratio;
    }
    void SetPosition(std::uint32_t handle,
                     const pal4::TimedMovePosition& position) override {
        Check(handle == player, "player position setter receiver");
        written = position;
        calls.emplace_back("set position");
    }
    void CloseWait(
        pal4::CurrentPlayerTimedMoveWaitState& handler) override {
        Check(&handler == &state, "player timed move close receiver");
        calls.emplace_back("close");
    }
};

void CreateCases() {
    const pal4::TimedMovePosition target{5.0f, 6.0f, 7.0f};
    Runtime runtime;
    pal4::ScriptCurrentPlayerTimedMove move(runtime);
    Check(move.Create(2.0f, target, 7u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "current", "movement", "animation name",
                  "select C01", "flag 1", "allocate", "executor",
                  "construct", "read position", "flush", "executor",
                  "add handler"},
          "current player changes animation before timed movement");

    Runtime already_c01;
    already_c01.animation = "c01";
    pal4::ScriptCurrentPlayerTimedMove same_action(already_c01);
    Check(same_action.Create(2.0f, target, 7u) == 0x1234u &&
              already_c01.calls == std::vector<std::string>{
                  "manager", "current", "movement", "animation name",
                  "allocate", "executor", "construct", "read position",
                  "flush", "executor", "add handler"},
          "case-insensitive C01 match skips animation change");

    Runtime no_player;
    no_player.player = 0u;
    pal4::ScriptCurrentPlayerTimedMove missing(no_player);
    Check(missing.Create(2.0f, target, 7u) == 0u &&
              no_player.calls == std::vector<std::string>{
                  "manager", "current"},
          "missing current player skips movement");

    Runtime no_movement;
    no_movement.movement = false;
    pal4::ScriptCurrentPlayerTimedMove inactive(no_movement);
    Check(inactive.Create(2.0f, target, 7u) == 0u &&
              no_movement.calls == std::vector<std::string>{
                  "manager", "current", "movement"},
          "missing movement component skips animation and wait");

    Runtime zero;
    pal4::ScriptCurrentPlayerTimedMove zero_move(zero);
    Check(zero_move.Create(0.0f, target, 7u) == 0x1234u &&
              zero.state.vtable_address == 0x849684u &&
              zero.calls == std::vector<std::string>{
                  "manager", "current", "movement", "animation name",
                  "select C01", "flag 1", "allocate", "executor",
                  "construct", "executor", "add handler"},
          "nonpositive duration queues unflushed wait");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptCurrentPlayerTimedMove failed_move(failed);
    Check(failed_move.Create(2.0f, target, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "current", "movement", "animation name",
                  "select C01", "flag 1", "allocate", "executor",
                  "add handler"},
          "failed allocation queues null after animation selection");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptCurrentPlayerTimedMove move(runtime);
    auto& state = runtime.state;
    state.object = runtime.player;
    state.start = runtime.initial;
    state.target = {5.0f, 6.0f, 7.0f};
    state.duration = 2.0f;
    move.Update(state, 0.5);
    Check(state.elapsed == 0.5f &&
              runtime.written == pal4::TimedMovePosition{2, 3, 4} &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "interpolate", "interpolate",
                  "set position"},
          "player partial movement interpolates XYZ");
    runtime.calls.clear();
    move.Update(state, 1.5);
    Check(state.elapsed == 2.0f && runtime.written == state.target &&
              runtime.calls == std::vector<std::string>{
                  "close", "interpolate", "interpolate", "interpolate",
                  "set position"},
          "player timed movement closes before exact final position");
    state.duration = 0.0f;
    runtime.calls.clear();
    move.Update(state, 0.5);
    Check(runtime.calls.empty(),
          "nonpositive duration update does not close or move");
}

}  // namespace

int main() {
    try {
        CreateCases();
        UpdateCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
