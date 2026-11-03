#include "pal4/script_object_action.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptObjectActionRuntime {
public:
    pal4::ObjectActionWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t object = 0x5000u;
    std::uint32_t type = 6u;
    bool allocation_fails = false;
    bool flag = false;
    std::string animation = "Walk";
    std::string label;

    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x4000u && name == "box", "object lookup");
        calls.emplace_back("find");
        return object;
    }
    std::uint32_t ObjectType(std::uint32_t handle) override {
        Check(handle == object, "object type receiver");
        calls.emplace_back("type");
        return type;
    }
    std::string_view AnimationName(std::uint32_t handle) override {
        Check(handle == object, "animation name receiver");
        calls.emplace_back("name");
        return animation;
    }
    void SelectAnimation(std::uint32_t handle,
                         std::string_view name) override {
        Check(handle == object && name == "run", "selected animation");
        calls.emplace_back("select");
    }
    void SetAnimationTime(std::uint32_t handle, float time) override {
        Check(handle == object, "animation time receiver");
        calls.emplace_back("time:" + std::to_string(time));
    }
    void ClearAnimationFlag(std::uint32_t handle,
                            std::uint32_t bit) override {
        Check(handle == object && bit <= 1u, "clear animation bit");
        calls.emplace_back("clear:" + std::to_string(bit));
    }
    void SetAnimationFlag(std::uint32_t handle,
                          std::uint32_t bit) override {
        Check(handle == object && bit == 1u, "set animation bit");
        calls.emplace_back("set:1");
    }
    bool AnimationFlag(std::uint32_t handle) override {
        Check(handle == object, "animation flag receiver");
        calls.emplace_back("flag");
        return flag;
    }
    float AnimationLength(std::uint32_t handle) override {
        Check(handle == object, "animation length receiver");
        calls.emplace_back("length");
        return 2.5f;
    }
    pal4::ObjectActionWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x2cu, "wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::ObjectActionWaitState& handler,
                       std::uint8_t mode, std::uint32_t queue) override {
        Check(&handler == &state && mode == 7u && queue == 0x6054u,
              "wait constructor");
        calls.emplace_back("construct");
    }
    void AssignName(pal4::ObjectActionWaitState& handler,
                    std::string_view name) override {
        Check(&handler == &state && name == "box-OBJ-Action",
              "handler name");
        Check(state.vtable_address == 0x8495f4u && state.phase == 4u &&
                  state.object == object && state.target_loops == 2 &&
                  state.completed_loops == 0,
              "handler layout before name assignment");
        label = name;
        calls.emplace_back("assign");
    }
    void FlushWait(pal4::ObjectActionWaitState& handler) override {
        Check(&handler == &state, "flush receiver");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::ObjectActionWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "queue handler");
        calls.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup name");
        return 0x1234u;
    }
    void CloseWait(pal4::ObjectActionWaitState& handler) override {
        Check(&handler == &state, "close receiver");
        calls.emplace_back("close");
    }
};

void CreateCases() {
    Runtime same;
    pal4::ScriptObjectAction action(same);
    Check(action.Create("box", "wALK", 2, 7u) == 0x1234u,
          "create return from name cleanup");
    Check(same.calls == std::vector<std::string>{
              "manager", "find", "type", "name", "time:0.000000",
              "clear:0", "set:1", "allocate", "executor", "construct",
              "assign", "flush", "executor", "add", "cleanup name"},
          "same animation resets time and queues wait");

    Runtime changed;
    pal4::ScriptObjectAction changed_action(changed);
    Check(changed_action.Create("box", "run", 2, 7u) == 0x1234u,
          "different animation cleanup return");
    Check(changed.calls[4] == "select" &&
              changed.label == "box-OBJ-Action",
          "different animation selects by name");

    Runtime wrong_type;
    wrong_type.type = 9u;
    pal4::ScriptObjectAction wrong_action(wrong_type);
    Check(wrong_action.Create("box", "run", 2, 7u) == 9u &&
              wrong_type.calls == std::vector<std::string>{
                  "manager", "find", "type"},
          "wrong object type returns native type result");

    Runtime missing;
    missing.object = 0u;
    pal4::ScriptObjectAction missing_action(missing);
    bool threw = false;
    try {
        static_cast<void>(missing_action.Create("box", "run", 2, 7u));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Check(threw && missing.calls == std::vector<std::string>{
              "manager", "find"},
          "missing object exposes original invalid-object precondition");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptObjectAction failed_action(failed);
    Check(failed_action.Create("box", "run", 2, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "find", "type", "name", "select",
                  "clear:0", "set:1", "allocate", "executor", "add",
                  "cleanup name"},
          "failed allocation still queues null and cleans formatted name");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptObjectAction action(runtime);
    auto& state = runtime.state;
    state.object = runtime.object;
    state.target_loops = 2;
    action.Update(state);
    Check(state.completed_loops == 0 &&
              runtime.calls == std::vector<std::string>{"flag"},
          "positive target waits for animation flag");
    runtime.flag = true;
    action.Update(state);
    action.Update(state);
    Check(state.completed_loops == 2 &&
              runtime.calls == std::vector<std::string>{
                  "flag", "flag", "flag"},
          "two flagged updates count two loops");
    runtime.calls.clear();
    action.Update(state);
    Check(state.completed_loops == 0 &&
              runtime.calls == std::vector<std::string>{
                  "time:0.000000", "clear:1", "close"},
          "completed positive target resets time and closes");

    state.target_loops = -1;
    state.completed_loops = 5;
    runtime.calls.clear();
    runtime.flag = false;
    action.Update(state);
    Check(state.completed_loops == 5 &&
              runtime.calls == std::vector<std::string>{"flag"},
          "minus-one target waits for animation flag");
    runtime.calls.clear();
    runtime.flag = true;
    action.Update(state);
    Check(state.completed_loops == 0 &&
              runtime.calls == std::vector<std::string>{
                  "flag", "length", "time:2.500000", "clear:1", "close"},
          "minus-one target seeks to animation length and closes");

    state.target_loops = 0;
    runtime.calls.clear();
    action.Update(state);
    Check(runtime.calls.empty(), "zero target remains idle");
    state.target_loops = -2;
    action.Update(state);
    Check(runtime.calls.empty(), "other negative target remains idle");

    state.completed_loops = 7;
    action.Cleanup(state);
    Check(state.completed_loops == 0 &&
              runtime.calls == std::vector<std::string>{"close"},
          "cleanup resets count and closes without animation writes");
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
