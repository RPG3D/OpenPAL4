#include "pal4/game_section_executor.h"
#include "pal4/pal_actor_control_interaction.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlInteractionRuntime {
    std::vector<std::string> events;
    bool cache_hit = false;
    std::uint32_t interpreter = 0x500u;
    bool execution_exception = false;
    std::string expected_section = "OnEnter";

    void InitializeScripts() override {
        events.emplace_back("initialize");
    }
    std::string ArenaScriptPath() override {
        events.emplace_back("path");
        return "scripts/scene.cs";
    }
    std::uint32_t ArenaScriptContext() override {
        events.emplace_back("context");
        return 0xaau;
    }
    std::uint32_t CsbMode() override {
        events.emplace_back("mode");
        return 1u;
    }
    bool CacheContains(std::string_view key) override {
        Check(key == "scripts/scene.cs[" + expected_section + "]",
              "script cache key uses path and bracketed section");
        events.emplace_back("cache");
        return cache_hit;
    }
    std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::string_view path,
        std::string_view section, std::string_view name,
        std::uint32_t context, std::uint32_t callback) override {
        Check(mode == 1u && path == "scripts/scene.cs" &&
                  section == expected_section && name == expected_section &&
                  context == 0xaau &&
                  callback == pal4::GameSectionExecutor::kCsbReadCallback,
              "script interpreter creation arguments");
        events.emplace_back("create");
        return interpreter;
    }
    bool HandleExecutionException(std::uint32_t value) override {
        Check(value == interpreter, "execution interpreter");
        events.emplace_back("handle");
        return execution_exception;
    }
    std::string FunctionName(std::uint32_t value) override {
        Check(value == interpreter, "function-name interpreter");
        events.emplace_back("function");
        return "OnEnter";
    }
    std::string ExceptionWhat(std::uint32_t value) override {
        Check(value == interpreter, "exception interpreter");
        events.emplace_back("what");
        return "failed";
    }
    void ReportScriptException(std::string_view what,
                               std::string_view function) override {
        Check(what == "failed" && function == "OnEnter",
              "script exception report arguments");
        events.emplace_back("report");
    }
    void ReleaseException(std::uint32_t value) override {
        Check(value == interpreter, "released interpreter");
        events.emplace_back("release");
    }
    void ShowMissingScript(std::string_view message,
                           std::string_view title) override {
        Check(message == "not found script files" &&
                  title == "no script", "missing-script dialog text");
        events.emplace_back("missing");
    }
    void CleanupScripts() override {
        events.emplace_back("cleanup");
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x777u, "script text address");
        events.emplace_back("read");
        return expected_section;
    }
};

void TestCacheAndFailures() {
    Runtime runtime;
    runtime.cache_hit = true;
    Check(pal4::GameSectionExecutor::Execute("OnEnter", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "initialize", "path", "cache", "cleanup"},
          "cache hit skips interpreter construction");
    runtime = {};
    runtime.interpreter = 0u;
    Check(pal4::GameSectionExecutor::Execute("OnEnter", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "initialize", "path", "cache", "context", "mode",
                  "create", "missing", "cleanup"},
          "null interpreter displays missing-script message");
    runtime = {};
    Check(pal4::GameSectionExecutor::Execute("OnEnter", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "initialize", "path", "cache", "context", "mode",
                  "create", "handle", "missing", "cleanup"},
          "false execution result displays missing-script message");
}

void TestExceptionAndDefaultBinding() {
    Runtime runtime;
    runtime.execution_exception = true;
    Check(pal4::GameSectionExecutor::Execute("OnEnter", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "initialize", "path", "cache", "context", "mode",
                  "create", "handle", "function", "what", "report",
                  "release", "cleanup"},
          "execution exception is reported and released in order");
    runtime = {};
    runtime.expected_section = "Intro";
    runtime.cache_hit = true;
    constexpr char section[] = {'I', 'n', 't', 'r', 'o', '\0', 'X'};
    Check(pal4::GameSectionExecutor::Execute(
              std::string_view(section, sizeof(section)), runtime) == 0u &&
              runtime.events.back() == "cleanup",
          "script section stops at original C-string NUL");
    runtime = {};
    runtime.cache_hit = true;
    runtime.ExecuteScript(0x777u);
    Check(runtime.events == std::vector<std::string>{
              "read", "initialize", "path", "cache", "cleanup"},
          "interaction runtime delegates script execution to class");
}

}  // namespace

int main() {
    TestCacheAndFailures();
    TestExceptionAndDefaultBinding();
}
