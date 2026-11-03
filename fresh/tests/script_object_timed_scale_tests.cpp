#include "pal4/script_object_timed_scale.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptObjectTimedScaleRuntime {
public:
    pal4::TimedObjectScaleWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t object = 0x5000u;
    bool animated = true;
    bool allocation_fails = false;
    pal4::TimedMovePosition initial{1.0f, 2.0f, 3.0f};
    pal4::TimedMovePosition written{};

    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x4000u && name == "box", "scale lookup");
        calls.emplace_back("find");
        return object;
    }
    bool HasAnimatedObject(std::uint32_t handle) override {
        Check(handle == object, "scale animation receiver");
        calls.emplace_back("animated");
        return animated;
    }
    pal4::TimedMovePosition ReadScale(
        std::uint32_t handle) override {
        Check(handle == object, "scale read receiver");
        calls.emplace_back("read scale");
        return initial;
    }
    pal4::TimedObjectScaleWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x44u, "scale wait size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::TimedObjectScaleWaitState& handler,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && mode == 7u && queue == 0x6054u,
              "scale wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::TimedObjectScaleWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x849678u &&
                  handler.object == object &&
                  handler.target == pal4::TimedMovePosition{5, 5, 5} &&
                  handler.start == initial &&
                  handler.duration == 2.0f && handler.elapsed == 0.0f,
              "scale wait state before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue,
        pal4::TimedObjectScaleWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "scale wait queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
    float Interpolate(float start, float end,
                       float ratio) override {
        Check(ratio == 0.25f || ratio == 1.0f,
              "scale interpolation ratio");
        calls.emplace_back("interpolate");
        return start + (end - start) * ratio;
    }
    void SetScale(std::uint32_t handle,
                  const pal4::TimedMovePosition& scale,
                  std::uint32_t mode) override {
        Check(handle == object && mode == 0u, "scale setter operands");
        written = scale;
        calls.emplace_back("set scale");
    }
    void CloseWait(pal4::TimedObjectScaleWaitState& handler) override {
        Check(&handler == &state, "scale close receiver");
        calls.emplace_back("close");
    }
};

void CreateCases() {
    Runtime runtime;
    pal4::ScriptObjectTimedScale scale(runtime);
    Check(scale.Create("box", 5.0f, 2.0f, 7u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "find", "animated", "allocate",
                  "executor", "construct", "read scale", "flush",
                  "executor", "add handler"},
          "positive duration stores uniform target and captured XYZ scale");

    Runtime missing;
    missing.object = 0u;
    pal4::ScriptObjectTimedScale missing_scale(missing);
    Check(missing_scale.Create("box", 5.0f, 2.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "find"}, "missing object skips scale");

    Runtime inactive;
    inactive.animated = false;
    pal4::ScriptObjectTimedScale inactive_scale(inactive);
    Check(inactive_scale.Create("box", 5.0f, 2.0f, 7u) == 0u &&
              inactive.calls == std::vector<std::string>{
                  "manager", "find", "animated"},
          "missing animated component returns zero");

    Runtime zero;
    pal4::ScriptObjectTimedScale zero_scale(zero);
    Check(zero_scale.Create("box", 5.0f, 0.0f, 7u) == 0x1234u &&
              zero.state.vtable_address == 0x849678u &&
              zero.calls == std::vector<std::string>{
                  "manager", "find", "animated", "allocate",
                  "executor", "construct", "executor", "add handler"},
          "nonpositive duration queues unflushed scale wait");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptObjectTimedScale failed_scale(failed);
    Check(failed_scale.Create("box", 5.0f, 2.0f, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "find", "animated", "allocate",
                  "executor", "add handler"},
          "allocation failure queues null");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptObjectTimedScale scale(runtime);
    auto& state = runtime.state;
    state.object = runtime.object;
    state.target = {5.0f, 5.0f, 5.0f};
    state.start = runtime.initial;
    state.duration = 2.0f;
    scale.Update(state, 0.5);
    Check(state.elapsed == 0.5f &&
              runtime.written == pal4::TimedMovePosition{2.0f, 2.75f, 3.5f} &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "interpolate", "interpolate",
                  "set scale"},
          "partial scale update interpolates all axes");

    runtime.calls.clear();
    scale.Update(state, 1.5);
    Check(state.elapsed == 2.0f &&
              runtime.written == state.target &&
              runtime.calls == std::vector<std::string>{
                  "close", "interpolate", "interpolate", "interpolate",
                  "set scale"},
          "completed scale closes before writing exact target");

    state.duration = 0.0f;
    runtime.calls.clear();
    scale.Update(state, 0.5);
    Check(runtime.calls.empty(),
          "nonpositive duration update returns without close");
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
