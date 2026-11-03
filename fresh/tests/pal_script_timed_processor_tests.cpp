#include "pal4/pal_script_timed_processor.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalScriptTimedRuntime {
    std::vector<std::string> calls;
    bool actor_active = true;
    bool stack_contains = true;
    bool module_result = true;
    bool throw_on_append = false;
    std::uint32_t executor_flag = 0u;
    std::uint32_t interpreter = 0x8000u;

    bool ActorControlActive() override {
        calls.emplace_back("actor");
        return actor_active;
    }
    std::uint32_t ExecutorAddress() override {
        calls.emplace_back("executor");
        return 0x5000u;
    }
    std::uint32_t ExecutorWord(const std::uint32_t address) override {
        Check(address == 0x5060u, "busy flag at executor +96");
        calls.emplace_back("busy");
        return executor_flag;
    }
    void InitializeScriptSystem() override {
        calls.emplace_back("script-init");
    }
    void InitializePath(pal4::StringBufferState& destination,
                        const char* text) override {
        Check(std::string(text) == "TimeScript", "script path literal");
        calls.emplace_back("path-init");
        destination = {0x1000u, 10u, 11u};
    }
    void CopyString(pal4::StringBufferState& destination,
                    const pal4::StringBufferState& source) override {
        Check(source.data == 0x1000u && source.length == 10u,
              "full path copies base path");
        calls.emplace_back("path-copy");
        destination = {0x2000u, 10u, 32u};
    }
    void AppendLiteral(pal4::StringBufferState& destination,
                       const char* text,
                       const std::uint32_t length) override {
        Check(destination.data == 0x2000u && length == 1u,
              "literal append targets full path");
        if (throw_on_append) throw std::runtime_error("append");
        calls.emplace_back(std::string("append:") + text);
        destination.length += length;
    }
    void AppendText(pal4::StringBufferState& destination,
                    const std::uint32_t text,
                    const std::uint32_t length) override {
        Check(destination.data == 0x2000u &&
                  text == 0x9000u && length == 4u,
              "callback text uses executor +72/+76");
        calls.emplace_back("append-callback");
        destination.length += length;
    }
    bool HasStackModule(const std::uint32_t context,
                             const std::uint32_t full_path) override {
        Check(context == 0x5054u && full_path == 0x2000u,
              "cache receives executor +84 and built full path");
        calls.emplace_back("stack-contains");
        return stack_contains;
    }
    std::uint32_t ArenaContext() override {
        calls.emplace_back("arena");
        return 0x7000u;
    }
    std::uint32_t ScriptMode() override {
        calls.emplace_back("mode");
        return 2u;
    }
    std::uint32_t CreateInterpreter(
        const pal4::TimedScriptInterpreterRequest& request) override {
        Check(request.mode == 2u && request.script_path == 0x1000u &&
                  request.full_path == 0x2000u &&
                  request.callback == 0x9000u &&
                  request.arena_context == 0x7000u &&
                  request.read_callback == 0x600d50u,
              "interpreter factory request preserves native operands");
        calls.emplace_back("create");
        return interpreter;
    }
    bool AddScriptModule(const std::uint32_t context,
                                  const std::uint32_t input) override {
        Check(context == 0x5054u && input == interpreter,
              "execution handler receives interpreter");
        calls.emplace_back("add-module");
        return module_result;
    }
    const char* FunctionName(const std::uint32_t input) override {
        Check(input == interpreter, "function name interpreter");
        calls.emplace_back("function");
        return "OnTime";
    }
    const char* FullPath(const std::uint32_t input) override {
        Check(input == interpreter, "full path interpreter");
        calls.emplace_back("full-path");
        return "TimeScript[OnTime]";
    }
    void PushStackModule(const std::uint32_t context, const char* full_path,
                         const char* function_name) override {
        Check(context == 0x5054u && std::string(full_path) == "TimeScript[OnTime]" &&
                  std::string(function_name) == "OnTime",
              "stack receives module path and entry function");
        calls.emplace_back("push-stack");
    }
    void ReleaseInterpreter(const std::uint32_t input) override {
        Check(input == interpreter, "release receives interpreter");
        calls.emplace_back("release");
    }
    void ShowMissingScriptMessage() override {
        calls.emplace_back("message-box");
    }
    void CleanupAllScripts() override {
        calls.emplace_back("cleanup-scripts");
    }
    std::uint32_t CleanupString(pal4::StringBufferState& text) override {
        if (text.data == 0x2000u) {
            calls.emplace_back("cleanup-full");
            return 0x5678u;
        }
        Check(text.data == 0x1000u, "cleanup path or full path");
        calls.emplace_back("cleanup-path");
        return 0x1234u;
    }
};

pal4::PalScriptExecutorState TimedState(const float delay) {
    pal4::PalScriptExecutorState state{};
    state.words[15u] = 1u;
    state.words[16u] = std::bit_cast<std::uint32_t>(0.0f);
    state.words[17u] = std::bit_cast<std::uint32_t>(delay);
    state.words[18u] = 0x9000u;
    state.words[19u] = 4u;
    return state;
}

void TestClockAndGate() {
    Runtime runtime;
    auto state = TimedState(4.0f);
    state.words[15u] = 0u;
    Check(pal4::PalScriptTimedProcessor::Process(state, 1.0f, runtime) == 0u &&
              runtime.calls.empty() && state.words[16u] == 0u,
          "disabled timer leaves elapsed time unchanged");

    state.words[15u] = 1u;
    state.words[16u] = std::bit_cast<std::uint32_t>(2.0f);
    Check(pal4::PalScriptTimedProcessor::Process(state, 1.0f, runtime) ==
              0x100u &&
              std::bit_cast<float>(state.words[16u]) == 3.0f &&
              runtime.calls.empty(),
          "timer accumulates before threshold without script work");

    state = TimedState(0.3f);
    state.words[16u] = std::bit_cast<std::uint32_t>(0.1f);
    Check(pal4::PalScriptTimedProcessor::Process(state, 0.2f, runtime) ==
              0x100u &&
              state.words[16u] == std::bit_cast<std::uint32_t>(0.3f),
          "threshold compares unrounded x87 sum before stored float");

    state = TimedState(1.0f);
    Check(pal4::PalScriptTimedProcessor::Process(
              state, std::numeric_limits<float>::quiet_NaN(), runtime) ==
              0x100u && runtime.calls.empty(),
          "unordered x87 comparison defers firing");

    state = TimedState(1.0f);
    runtime.actor_active = false;
    runtime.executor_flag = 1u;
    Check(pal4::PalScriptTimedProcessor::Process(state, 1.0f, runtime) ==
              0x5000u && state.words[16u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "actor", "executor", "busy"},
          "busy script executor blocks fired timer and preserves EAX");
}

void TestStackAndFactoryBranches() {
    Runtime cached;
    auto state = TimedState(1.0f);
    Check(pal4::PalScriptTimedProcessor::Process(state, 1.0f, cached) ==
              0x1234u &&
              cached.calls == std::vector<std::string>{
                  "actor", "script-init", "path-init", "path-copy",
                  "append:[", "append-callback", "append:]", "executor",
                  "stack-contains", "cleanup-scripts", "cleanup-full", "cleanup-path"},
          "active stack module skips creation and still cleans files and strings");

    Runtime missing;
    missing.stack_contains = false;
    missing.interpreter = 0u;
    state = TimedState(1.0f);
    Check(pal4::PalScriptTimedProcessor::Process(state, 1.0f, missing) ==
              0x1234u &&
              missing.calls == std::vector<std::string>{
                  "actor", "script-init", "path-init", "path-copy",
                  "append:[", "append-callback", "append:]", "executor",
                  "stack-contains", "arena", "mode", "create", "message-box",
                  "cleanup-scripts", "cleanup-full", "cleanup-path"},
          "missing interpreter displays message and cleans all resources");

    Runtime rejected;
    rejected.stack_contains = false;
    rejected.module_result = false;
    state = TimedState(1.0f);
    (void)pal4::PalScriptTimedProcessor::Process(state, 1.0f, rejected);
    Check(rejected.calls == std::vector<std::string>{
              "actor", "script-init", "path-init", "path-copy",
              "append:[", "append-callback", "append:]", "executor",
              "stack-contains", "arena", "mode", "create", "executor", "add-module",
              "message-box", "cleanup-scripts", "cleanup-full",
              "cleanup-path"},
          "failed module loading follows native missing-script branch");
}

void TestPushAndUnwind() {
    Runtime reported;
    reported.stack_contains = false;
    auto state = TimedState(1.0f);
    Check(pal4::PalScriptTimedProcessor::Process(state, 1.0f, reported) ==
              0x1234u &&
              reported.calls == std::vector<std::string>{
                  "actor", "script-init", "path-init", "path-copy",
                  "append:[", "append-callback", "append:]", "executor",
                  "stack-contains", "arena", "mode", "create", "executor", "add-module",
                  "executor", "function", "full-path", "push-stack", "release",
                  "cleanup-scripts", "cleanup-full", "cleanup-path"},
          "loaded interpreter pushes and releases before file cleanup");

    Runtime throwing;
    throwing.throw_on_append = true;
    state = TimedState(1.0f);
    bool caught = false;
    try {
        (void)pal4::PalScriptTimedProcessor::Process(state, 1.0f, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls == std::vector<std::string>{
              "actor", "script-init", "path-init", "path-copy",
              "cleanup-full", "cleanup-path"},
          "append failure unwinds constructed strings in reverse order");
}

}  // namespace

int main() {
    TestClockAndGate();
    TestStackAndFactoryBranches();
    TestPushAndUnwind();
}
