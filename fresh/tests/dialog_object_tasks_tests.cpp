#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectTaskRuntime {
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    std::vector<std::string> calls;
    bool enabled = true;
    bool active = false;
    std::optional<int> mode = 3;
    std::optional<float> wait = 1.5f;
    std::optional<float> expected = 2.5f;
    std::optional<std::string_view> class_name = "RotateTask";
    std::uint32_t created = 0x6000u;
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        calls.emplace_back("read:" + std::to_string(address) + ":" +
                           std::to_string(offset));
        return words.at({address, offset});
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(offset));
        words[{address, offset}] = value;
    }
    bool EffectsEnabled() override {
        calls.emplace_back("enabled"); return enabled;
    }
    bool EffectIsActive(std::uint32_t effect) override {
        Check(effect == 0x7000u, "activity effect");
        calls.emplace_back("active"); return active;
    }
    std::optional<int> IntAttribute(std::uint32_t xml,
                                     std::string_view name,
                                     bool ordered_tree) override {
        Check(xml == 0x4000u && name == "Attr_Task_Wait_Mode" &&
              ordered_tree == !enabled, "wait mode lookup path");
        calls.emplace_back(ordered_tree ? "mode:tree" : "mode:direct");
        return mode;
    }
    std::optional<float> FloatAttribute(std::uint32_t xml,
                                         std::string_view name) override {
        Check(xml == 0x4000u, "float XML parameter");
        if (name == "Attr_Task_Wait_Time") {
            calls.emplace_back("wait"); return wait;
        }
        Check(name == "Attr_Task_Expect_Time", "expected time name");
        calls.emplace_back("expected"); return expected;
    }
    std::optional<std::string_view> StringAttribute(
        std::uint32_t xml, std::string_view name) override {
        Check(xml == 0x4000u && name == "Attr_Class_Name",
              "task class name lookup");
        calls.emplace_back("name"); return class_name;
    }
    std::uint32_t CreateTask(std::uint32_t xml) override {
        Check(xml == 0x4000u, "task factory XML parameter");
        calls.emplace_back("create"); return created;
    }
    void AddTaskAndInitialize(
        std::uint32_t manager, std::uint32_t task, std::string_view name,
        const pal4::DialogTaskTiming& timing) override {
        Check(manager == 0x1000u && task == created &&
              name == class_name.value_or("") &&
              timing.wait_mode == mode.value_or(0) &&
              timing.wait_time == wait.value_or(0.0f) &&
              timing.expected_time == expected.value_or(0.0f),
              "task container arguments and defaults");
        calls.emplace_back("add");
    }
    void BindDialogObject(std::uint32_t task,
                          std::uint32_t object) override {
        Check(task == created && object == 0x8000u, "task owner binding");
        calls.emplace_back("bind");
    }
    void PrepareTask(std::uint32_t task) override {
        Check(task == created, "prepare task handle");
        calls.emplace_back("prepare");
    }
    void StartTask(std::uint32_t task) override {
        Check(task == created, "start task handle");
        calls.emplace_back("start");
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[2u] = 0x1000u;
    state.words[24u] = 0x2000u;
    state.words[25u] = 0x2008u;
    state.words[28u] = 0x5000u;
    state.words[32u] = 0x3000u;
    return state;
}
Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[{0x2000u, 0u}] = 1u;
    runtime.words[{0x3000u, 0u}] = 0x4000u;
    runtime.words[{0x5014u, 16u}] = 0x7000u;
    return runtime;
}
void ExpectError(pal4::DialogObjectState& state, Runtime& runtime,
                 pal4::DialogTaskInitError code) {
    try {
        (void)pal4::DialogObject::InitializeTasks(state, 0x8000u, runtime);
    } catch (const pal4::DialogTaskInitException& error) {
        Check(error.code() == code, "task initialization error code");
        if (code == pal4::DialogTaskInitError::MissingManager)
            Check(std::string_view(error.what()).starts_with("vtmMgr"),
                  "native missing-manager message");
        if (code == pal4::DialogTaskInitError::CreateTaskFailed)
            Check(std::string_view(error.what()).ends_with("RotateTask"),
                  "native creation error includes task name");
        return;
    }
    Check(false, "expected task initialization exception");
}
}  // namespace

int main() {
    auto state = State();
    auto runtime = MakeRuntime();
    state.words[2u] = 0u;
    ExpectError(state, runtime, pal4::DialogTaskInitError::MissingManager);
    Check(runtime.calls.empty(), "missing manager throws before task traversal");

    state = State();
    state.words[25u] = state.words[24u];
    Check(pal4::DialogObject::InitializeTasks(state, 0x8000u, runtime) &&
          runtime.calls.empty(), "empty task range succeeds");

    state = State();
    Check(pal4::DialogObject::InitializeTasks(state, 0x8000u, runtime) &&
          runtime.words.at({0x2000u, 4u}) == 0x6000u &&
          runtime.words.at({0x6000u, 64u}) == 0x7000u &&
          runtime.calls == std::vector<std::string>{
              "read:12288:0", "enabled", "mode:direct",
              "wait", "expected", "name", "create", "add", "write:8192:4",
              "read:8192:0", "read:20500:16", "write:24576:64",
              "bind", "prepare", "start"},
          "enabled task creation, effect binding and virtual call order");

    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.active = true;
    Check(pal4::DialogObject::InitializeTasks(state, 0x8000u, runtime) &&
          runtime.words.at({0x2000u, 4u}) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read:12288:0", "enabled", "read:8192:0",
              "read:20500:16", "active", "write:8192:4"},
          "disabled system skips already active effect");

    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.mode.reset();
    runtime.wait.reset();
    runtime.expected.reset();
    Check(pal4::DialogObject::InitializeTasks(state, 0x8000u, runtime) &&
          runtime.calls == std::vector<std::string>{
              "read:12288:0", "enabled", "read:8192:0",
              "read:20500:16", "active", "mode:tree", "wait",
              "expected", "name", "create", "add", "write:8192:4",
              "read:20500:16", "write:24576:64", "bind", "prepare", "start"},
          "disabled inactive effect uses tree mode and zero timing defaults");

    runtime = MakeRuntime();
    runtime.created = 0u;
    ExpectError(state, runtime, pal4::DialogTaskInitError::CreateTaskFailed);
    Check(runtime.words.at({0x2000u, 4u}) == 0u &&
          runtime.calls.back() == "write:8192:4",
          "enabled task creation failure records null before throwing");

    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.class_name.reset();
    ExpectError(state, runtime, pal4::DialogTaskInitError::EmptyClassName);
    Check(runtime.calls.back() == "name",
          "disabled empty class name throws before factory");

    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.created = 0u;
    ExpectError(state, runtime, pal4::DialogTaskInitError::EffectPlaybackFailed);
    Check(runtime.words.at({0x2000u, 4u}) == 0u &&
          runtime.calls.back() == "write:8192:4",
          "disabled task creation failure uses playback error");

    runtime = MakeRuntime();
    state.words[25u] = 0x2010u;
    runtime.words[{0x2000u, 4u}] = 0x6000u;
    runtime.words[{0x2008u, 4u}] = 0x7000u;
    Check(pal4::DialogObject::ResetTaskSlots(state, runtime) == 0x2010u &&
          runtime.words.at({0x2000u, 4u}) == 0u &&
          runtime.words.at({0x2008u, 4u}) == 0u &&
          runtime.calls == std::vector<std::string>{
              "write:8192:4", "write:8200:4"},
          "reset walks all task slots without changing record count");
    runtime.calls.clear();
    state.words[25u] = state.words[24u];
    Check(pal4::DialogObject::ResetTaskSlots(state, runtime) == 0x2000u &&
          runtime.calls.empty(), "reset empty range returns its end pointer");
}
