#include "pal4/minimap_scene_ui_timer_script.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMinimapSceneUiTimerScriptRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> texts;
    bool already_active = false;
    bool module_added = false;
    bool throw_on_first_text = false;
    bool throw_on_create = false;
    std::uint32_t interpreter = 0x9000u;
    std::uint32_t empty_log_calls = 0u;
    std::uint32_t engine_calls = 0u;
    std::uint32_t script_name = 0u;

    void SetWindowText(std::uint32_t window,
                       std::string_view text) override {
        Check(window == 0x7000u, "countdown window at owner +0xbc");
        texts.emplace_back(text);
        calls.emplace_back("text");
        if (throw_on_first_text && texts.size() == 1u) {
            throw std::runtime_error("set text");
        }
    }
    std::string_view EmptyErrorText() override {
        calls.emplace_back("empty-text");
        return "";
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame");
        return 0xa000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0xa1a0u, "timer queue slot");
        calls.emplace_back("queue");
        return 0xb000u;
    }
    void RemoveTimer(std::uint32_t queue,
                     std::uint32_t identifier) override {
        Check(queue == 0xb000u && identifier == 0x50d0u,
              "remove expired timer by scene identifier");
        calls.emplace_back("remove");
    }
    void InitializeScriptSystem() override {
        calls.emplace_back("script-init");
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0xc000u;
    }
    std::string CopyContext(std::uint32_t source) override {
        Check(source == 0xc0d4u, "arena script context at +0xd4");
        calls.emplace_back("context");
        return "zone";
    }
    std::uint32_t ScriptEngine() override {
        ++engine_calls;
        calls.emplace_back("engine");
        return 0xd000u;
    }
    bool HasStackModule(std::uint32_t context,
                        std::string_view path) override {
        Check(context == 0xd054u && path == "PFNTimerScript[TimerScript]",
              "active timer module key");
        calls.emplace_back("active-module");
        return already_active;
    }
    std::uint32_t EmptyLogMessage() override {
        ++empty_log_calls;
        calls.emplace_back("empty-log");
        return 0xe000u;
    }
    std::uint32_t ScriptMode() override {
        calls.emplace_back("mode");
        return 3u;
    }
    std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::string_view script_path,
        std::string_view full_path, std::uint32_t name,
        std::string_view context, std::uint32_t reader) override {
        Check(mode == 3u && script_path == "PFNTimerScript" &&
                  full_path == "PFNTimerScript[TimerScript]" &&
                  context == "zone" &&
                  reader == pal4::MinimapSceneUiTimerScript::kReaderFunction,
              "script interpreter arguments");
        script_name = name;
        calls.emplace_back("create");
        if (throw_on_create) {
            throw std::runtime_error("create interpreter");
        }
        return interpreter;
    }
    bool AddScriptModule(std::uint32_t context,
                                  std::uint32_t value) override {
        Check(context == 0xd054u && value == interpreter,
              "script execution context");
        calls.emplace_back("add");
        return module_added;
    }
    std::string_view FunctionName(std::uint32_t value) override {
        Check(value == interpreter, "module function interpreter");
        calls.emplace_back("function");
        return "OnTimer";
    }
    std::string_view FullPath(std::uint32_t value) override {
        Check(value == interpreter, "module path interpreter");
        calls.emplace_back("path");
        return "PFNTimerScript[TimerScript]";
    }
    void PushStackModule(
        std::uint32_t context, std::string_view full_path,
        std::string_view function_name) override {
        Check(context == 0xd054u && full_path == "PFNTimerScript[TimerScript]" &&
                  function_name == "OnTimer",
              "registered module queues its original full path and function");
        calls.emplace_back("push");
    }
    void ReleaseInterpreter(std::uint32_t value) override {
        Check(value == interpreter, "release temporary interpreter");
        calls.emplace_back("release");
    }
    void CleanupAllScripts() override {
        calls.emplace_back("cleanup-scripts");
    }
    std::uint32_t CleanupCountdown(std::string_view text) override {
        Check(text == texts.front(), "cleanup original formatted countdown");
        calls.emplace_back("cleanup-countdown");
        return 0x1234u;
    }
};

pal4::MinimapSceneUiState MakeState(std::int32_t countdown,
                                    std::uint32_t script_name = 0x8000u) {
    pal4::MinimapSceneUiState state;
    state.WriteWord(0xbcu, 0x7000u);
    state.WriteWord(0xe4u, script_name);
    state.WriteWord(0xf0u, static_cast<std::uint32_t>(countdown));
    return state;
}

std::uint32_t Execute(pal4::MinimapSceneUiState& state,
                      Runtime& runtime) {
    return pal4::MinimapSceneUiTimerScript::Execute(
        state, 0x5000u, runtime);
}

}  // namespace

int main() {
    {
        Runtime runtime;
        auto state = MakeState(2);
        Check(Execute(state, runtime) == 0x1234u &&
                  state.ReadWord(0xf0u) == 1u &&
                  runtime.texts == std::vector<std::string>{"2"} &&
                  runtime.calls == std::vector<std::string>{
                                       "text", "cleanup-countdown"},
              "positive countdown displays then decrements without script");
    }
    {
        Runtime runtime;
        auto state = MakeState(0);
        Check(Execute(state, runtime) == 0x1234u &&
                  state.ReadWord(0xf0u) == 0xffffffffu &&
                  runtime.texts == std::vector<std::string>{"0", ""} &&
                  runtime.script_name == 0x8000u &&
                  runtime.engine_calls == 2u &&
                  runtime.empty_log_calls == 0u &&
                  runtime.calls == std::vector<std::string>{
                                       "text", "empty-text", "text", "frame",
                                       "queue", "remove", "script-init", "arena",
                                       "context", "engine", "active-module", "mode",
                                       "create", "engine", "add",
                                       "cleanup-scripts", "cleanup-countdown"},
              "zero countdown expires but rejected module registration never pushes a script");
    }
    {
        Runtime runtime;
        runtime.already_active = true;
        auto state = MakeState(-1);
        Check(Execute(state, runtime) == 0x1234u &&
                  state.ReadWord(0xf0u) == 0xfffffffeu &&
                  runtime.texts == std::vector<std::string>{"-1", ""} &&
                  runtime.engine_calls == 1u &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "cleanup-scripts" &&
                  runtime.calls.back() == "cleanup-countdown",
              "negative countdown formats signed and an active module suppresses duplicate preparation");
    }
    {
        Runtime runtime;
        runtime.interpreter = 0u;
        auto state = MakeState(0, 0u);
        Check(Execute(state, runtime) == 0x1234u &&
                  runtime.empty_log_calls == 1u &&
                  runtime.script_name == 0xe000u &&
                  runtime.engine_calls == 1u &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "cleanup-scripts",
              "null script name and null interpreter fallback");
    }
    {
        Runtime runtime;
        runtime.module_added = true;
        auto state = MakeState(0);
        Check(Execute(state, runtime) == 0x1234u &&
                  runtime.engine_calls == 3u &&
                  runtime.calls[runtime.calls.size() - 6u] == "function" &&
                  runtime.calls[runtime.calls.size() - 5u] == "path" &&
                  runtime.calls[runtime.calls.size() - 4u] == "push" &&
                  runtime.calls[runtime.calls.size() - 3u] == "release" &&
                  runtime.calls[runtime.calls.size() - 2u] == "cleanup-scripts",
              "accepted module queues function/path then releases interpreter before script cleanup");
    }
    for (const auto failure : {0, 1}) {
        Runtime runtime;
        runtime.throw_on_first_text = failure == 0;
        runtime.throw_on_create = failure == 1;
        auto state = MakeState(0);
        bool caught = false;
        try {
            (void)Execute(state, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls.back() == "cleanup-countdown" &&
                  state.ReadWord(0xf0u) ==
                      (failure == 0 ? 0u : 0xffffffffu),
              "formatted countdown cleans up on UI and interpreter errors");
    }
}
