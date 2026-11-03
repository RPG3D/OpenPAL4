#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_event.h"
#include "pal4/gob_obstacle_snapshot.h"
#include "gob_obstacle_snapshot_runtime.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobObstacleEventRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    ObstacleSnapshotRuntime snapshot;
    std::uint32_t interpreter = 0x3000u;
    std::uint32_t reset_result = 0x66u;
    bool cached = false;
    bool handled = true;
    std::string entry = "OnTouch";
    std::string name = "Barrier";

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address - object));
        return words[address];
    }
    std::string_view ReadCString(std::uint32_t address) override {
        calls.emplace_back("read string");
        if (address == 0x5000u) return entry;
        if (address == 0x6000u) return name;
        throw std::runtime_error("unknown string address");
    }
    void InitializeScript() override { calls.emplace_back("script init"); }
    std::string CurrentSceneScriptPath() override {
        calls.emplace_back("script path");
        return "scene.cs";
    }
    bool ScriptCacheContains(std::string_view key) override {
        Check(key == "scene.cs[OnTouch]", "script cache key");
        calls.emplace_back("cache check");
        return cached;
    }
    std::uint32_t CreateScriptInterpreter(
        std::string_view path, std::string_view key,
        std::string_view script_entry) override {
        Check(path == "scene.cs" && key == "scene.cs[OnTouch]" &&
              script_entry == "OnTouch", "interpreter arguments");
        calls.emplace_back("create interpreter");
        return interpreter;
    }
    bool HandleScriptExecution(std::uint32_t address) override {
        Check(address == interpreter, "handle interpreter address");
        calls.emplace_back("handle execution");
        return handled;
    }
    void ReportScriptException(std::uint32_t address) override {
        Check(address == interpreter, "report interpreter address");
        calls.emplace_back("report exception");
    }
    void ReleaseScriptException(std::uint32_t address) override {
        Check(address == interpreter, "release interpreter address");
        calls.emplace_back("release exception");
    }
    void ShowScriptNotFound() override { calls.emplace_back("not found"); }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup packages");
    }
    void CleanupScriptPath() override {
        calls.emplace_back("cleanup path");
    }
    void TryTriggerTaskRun(std::uint32_t address,
                           std::uint32_t event) override {
        Check(address == object && event == 0x44u,
              "task trigger arguments");
        calls.emplace_back("trigger task");
    }
    std::uint32_t ConsumeResearchUse(std::uint32_t address) override {
        Check(address == object, "consume object address");
        calls.emplace_back("consume use");
        return 0x99u;
    }
    std::uint32_t ResetMachineTriggeredRuntime(
        std::uint32_t address) override {
        Check(address == object, "reset object address");
        calls.emplace_back("reset machine");
        return reset_result;
    }
    pal4::IGobSnapshotRuntime& SnapshotRuntime() override {
        calls.emplace_back("snapshot runtime");
        return snapshot;
    }
};

void EnableScript(Runtime& runtime) {
    runtime.words[Runtime::object + 0x198u] = 1u;
    runtime.words[Runtime::object + 0x17cu] = 7u;
    runtime.words[Runtime::object + 0x178u] = 0x5000u;
}

void CheckEvent() {
    Runtime disabled;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, disabled) == 0u &&
          disabled.calls == (std::vector<std::string>{"read 408"}),
          "zero use count bypasses script and tasks");
    Runtime no_entry;
    no_entry.words[Runtime::object + 0x198u] = 1u;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, no_entry) == 0x99u &&
          no_entry.calls == (std::vector<std::string>{
              "read 408", "read 380", "trigger task",
              "read 608", "consume use"}),
          "empty script still triggers machine and consumes use");
    Runtime cached;
    EnableScript(cached);
    cached.cached = true;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, cached) == 0x99u &&
          cached.calls == (std::vector<std::string>{
              "read 408", "read 380", "script init", "script path",
              "read 376", "read string", "cache check",
              "cleanup packages", "cleanup path", "trigger task",
              "read 608", "consume use"}),
          "cache hit still reaches task trigger and consumption");
    Runtime handled;
    EnableScript(handled);
    handled.words[Runtime::object + 0x260u] = 3u;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, handled) == 3u &&
          handled.calls == (std::vector<std::string>{
              "read 408", "read 380", "script init", "script path",
              "read 376", "read string", "cache check",
              "create interpreter", "handle execution", "report exception",
              "release exception", "cleanup packages", "cleanup path",
              "trigger task", "read 608"}),
          "handled interpreter and pending task bypass consumption");
    Runtime missing;
    EnableScript(missing);
    missing.interpreter = 0u;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, missing) == 0x99u &&
          missing.calls == (std::vector<std::string>{
              "read 408", "read 380", "script init", "script path",
              "read 376", "read string", "cache check",
              "create interpreter", "not found", "cleanup packages",
              "cleanup path", "trigger task", "read 608", "consume use"}),
          "missing interpreter reports error then triggers tasks");
    Runtime unhandled;
    EnableScript(unhandled);
    unhandled.handled = false;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, unhandled) == 0x99u &&
          unhandled.calls[9] == "not found",
          "unhandled interpreter reports missing script");
    Runtime negative;
    negative.words[Runtime::object + 0x198u] = 1u;
    negative.words[Runtime::object + 0x260u] = 0xffffffffu;
    Check(pal4::GobObstacle::RunScriptAndProcessEvent(
              Runtime::object, 0x44u, negative) == 0x99u,
          "nonpositive signed task count consumes use");
    Runtime reset;
    Check(pal4::GobObstacle::ResetTriggeredRuntime(
              Runtime::object, reset) == 0x66u &&
          reset.calls == (std::vector<std::string>{"reset machine"}),
          "reset thunk forwards result");
}

void CheckSnapshotCapture() {
    Runtime disabled;
    Check(pal4::GobObstacle::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
          disabled.calls == (std::vector<std::string>{"read 420"}),
          "store flag gates snapshot");
    for (const std::string name : {std::string{}, std::string("A"),
                                   std::string(80, 'x')}) {
        Runtime runtime;
        runtime.name = name;
        runtime.words[Runtime::object + 0x1a4u] = 1u;
        runtime.words[Runtime::object + 4u] = 0x6000u;
        Check(pal4::GobObstacle::CreateStateSnapshot(
                  Runtime::object, runtime) ==
                  ObstacleSnapshotRuntime::snapshot &&
              runtime.snapshot.name == name &&
              runtime.snapshot.words[ObstacleSnapshotRuntime::snapshot] ==
                  pal4::GobObstacleStateSnapshot::kVtable,
              "snapshot captures empty, short and long names");
    }
    Runtime no_memory;
    no_memory.words[Runtime::object + 0x1a4u] = 1u;
    no_memory.snapshot.allocation = 0u;
    Check(pal4::GobObstacle::CreateStateSnapshot(
              Runtime::object, no_memory) == 0u &&
          no_memory.calls == (std::vector<std::string>{
              "read 420", "snapshot runtime"}),
          "snapshot allocation failure stops name copy");
}

}  // namespace

int main() {
    try {
        CheckEvent();
        CheckSnapshotCapture();
        std::cout << "gob_obstacle_event_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
