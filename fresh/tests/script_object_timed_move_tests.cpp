#include "pal4/script_object_timed_move.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptObjectTimedMoveRuntime {
public:
    pal4::TimedObjectMoveWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t object = 0x5000u;
    bool animated = true;
    bool allocation_fails = false;
    std::string label;
    pal4::TimedMovePosition written{};
    pal4::TimedMovePosition initial{1.0f, 2.0f, 3.0f};

    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x4000u && name == "box", "timed move lookup");
        calls.emplace_back("find");
        return object;
    }
    bool HasAnimatedObject(std::uint32_t handle) override {
        Check(handle == object, "timed move animation receiver");
        calls.emplace_back("animated");
        return animated;
    }
    pal4::TimedMovePosition ReadPosition(
        std::uint32_t handle) override {
        Check(handle == object, "timed move position receiver");
        calls.emplace_back("read position");
        return initial;
    }
    pal4::TimedObjectMoveWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x44u, "timed move wait size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::TimedObjectMoveWaitState& handler,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && mode == 7u && queue == 0x6054u,
              "timed move wait constructor");
        calls.emplace_back("construct");
    }
    void AssignName(pal4::TimedObjectMoveWaitState& handler,
                    std::string_view name) override {
        Check(&handler == &state && name == "box-OBJ-Movment" &&
                  handler.vtable_address == 0x849660u &&
                  handler.phase == 5u, "timed move wait name");
        label = name;
        calls.emplace_back("assign name");
    }
    void FlushWait(pal4::TimedObjectMoveWaitState& handler) override {
        Check(&handler == &state && handler.object == object &&
                  handler.target == pal4::TimedMovePosition{5, 6, 7} &&
                  handler.start == initial &&
                  handler.duration == 2.0f && handler.elapsed == 0.0f,
              "timed move state before flush");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::TimedObjectMoveWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "timed move queue");
        calls.emplace_back("add handler");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup name");
        return 0x1234u;
    }
    float Interpolate(float start, float end,
                       float ratio) override {
        Check(ratio == 0.25f || ratio == 1.0f,
              "timed move interpolation ratio");
        calls.emplace_back("interpolate");
        return start + (end - start) * ratio;
    }
    void SetPosition(std::uint32_t handle,
                     const pal4::TimedMovePosition& position,
                     std::uint32_t mode) override {
        Check(handle == object && mode == 0u,
              "timed move position setter");
        written = position;
        calls.emplace_back("set position");
    }
    void CloseWait(pal4::TimedObjectMoveWaitState& handler) override {
        Check(&handler == &state, "timed move close receiver");
        calls.emplace_back("close");
    }
};

void CreateCases() {
    const pal4::TimedMovePosition target{5.0f, 6.0f, 7.0f};
    Runtime runtime;
    pal4::ScriptObjectTimedMove move(runtime);
    Check(move.Create("box", 2.0f, target, 7u) == 0x1234u &&
              runtime.label == "box-OBJ-Movment" &&
              runtime.calls == std::vector<std::string>{
                  "manager", "find", "animated", "allocate",
                  "executor", "construct", "assign name",
                  "read position", "flush", "executor", "add handler",
                  "cleanup name"},
          "positive duration captures starting position and queues move");

    Runtime missing;
    missing.object = 0u;
    pal4::ScriptObjectTimedMove missing_move(missing);
    Check(missing_move.Create("box", 2.0f, target, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "find"},
          "missing object returns null without formatting wait name");

    Runtime inactive;
    inactive.animated = false;
    pal4::ScriptObjectTimedMove inactive_move(inactive);
    Check(inactive_move.Create("box", 2.0f, target, 7u) == 0x5000u &&
              inactive.calls == std::vector<std::string>{
                  "manager", "find", "animated"},
          "object without animated component returns original object");

    Runtime zero;
    pal4::ScriptObjectTimedMove zero_move(zero);
    Check(zero_move.Create("box", 0.0f, target, 7u) == 0x1234u &&
              zero.state.vtable_address == 0x849660u &&
              zero.calls == std::vector<std::string>{
                  "manager", "find", "animated", "allocate",
                  "executor", "construct", "executor", "add handler",
                  "cleanup name"},
          "nonpositive duration queues unflushed wait without movement data");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptObjectTimedMove failed_move(failed);
    Check(failed_move.Create("box", 2.0f, target, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "find", "animated", "allocate",
                  "executor", "add handler", "cleanup name"},
          "failed allocation still queues null and cleans name");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptObjectTimedMove move(runtime);
    auto& state = runtime.state;
    state.object = runtime.object;
    state.start = runtime.initial;
    state.target = {5.0f, 6.0f, 7.0f};
    state.duration = 2.0f;
    move.Update(state, 0.5);
    Check(state.elapsed == 0.5f &&
              runtime.written == pal4::TimedMovePosition{2, 3, 4} &&
              runtime.calls == std::vector<std::string>{
                  "animated", "interpolate", "interpolate",
                  "interpolate", "set position"},
          "partial update interpolates three coordinates");

    runtime.calls.clear();
    move.Update(state, 1.5);
    Check(state.elapsed == 2.0f &&
              runtime.written == state.target &&
              runtime.calls == std::vector<std::string>{
                  "animated", "close", "interpolate", "interpolate",
                  "interpolate", "set position"},
          "completed update closes before writing exact target");

    state.duration = 0.0f;
    runtime.calls.clear();
    move.Update(state, 0.5);
    Check(runtime.calls == std::vector<std::string>{"close"},
          "zero duration closes without component lookup");

    state.duration = 2.0f;
    runtime.animated = false;
    runtime.calls.clear();
    move.Update(state, 0.5);
    Check(runtime.calls == std::vector<std::string>{
              "animated", "close"},
          "missing animated component closes without position write");
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
