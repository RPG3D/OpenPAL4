#include "pal4/game_world_scene_script_loader.h"

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

using Temp = pal4::GameWorldScriptTemporary;

struct Runtime final : pal4::IGameWorldSceneScriptRuntime {
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<bool> modes{false, false};
    std::size_t mode_reads = 0u;
    bool module_active = false;
    std::uint32_t interpreter = 0x7000u;
    bool registration_succeeded = true;
    bool push_failed = false;
    std::string module_key;
    pal4::GameWorldSceneScriptRequest request;
    std::string missing_message;
    std::string missing_title;

    std::string_view ReadText(std::uint32_t address) override {
        events.push_back("read-text:" + std::to_string(address));
        return texts[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read-word:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back("write-word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteStoredText(std::uint32_t address,
                         std::string_view text) override {
        events.push_back("store:" + std::to_string(address));
        texts[address] = text;
    }
    std::string_view InitialFilePrefix() override {
        events.push_back("file-prefix");
        return "folder/";
    }
    bool IsCsbMode() override {
        events.push_back("mode");
        const auto index = mode_reads++;
        return index < modes.size() ? modes[index] : false;
    }
    void InitializeScriptSystem() override {
        events.push_back("initialize");
    }
    std::uint32_t GetExecutor() override {
        events.push_back("executor");
        return 0x9000u;
    }
    bool HasStackModule(std::uint32_t executor,
                       std::string_view path) override {
        Check(executor == 0x9000u, "active module lookup uses executor context");
        events.push_back("stack-module");
        module_key = path;
        return module_active;
    }
    std::uint32_t CreateInterpreter(
        const pal4::GameWorldSceneScriptRequest& value) override {
        events.push_back("create");
        request = value;
        return interpreter;
    }
    bool AddScriptModule(std::uint32_t executor,
                 std::uint32_t handle) override {
        Check(executor == 0x9000u && handle == interpreter,
              "module registration gets executor and interpreter handle");
        events.push_back("register");
        return registration_succeeded;
    }
    std::string_view InterpreterFunctionName(
        std::uint32_t handle) override {
        Check(handle == interpreter, "function name comes from interpreter");
        events.push_back("function-name");
        return "script-function";
    }
    std::string_view InterpreterFullPath(
        std::uint32_t handle) override {
        Check(handle == interpreter, "full path comes from interpreter");
        events.push_back("full-path");
        return "script-full-path";
    }
    std::int32_t PushStackModule(std::uint32_t executor,
                         std::string_view full_path,
                         std::string_view function_name) override {
        Check(executor == 0x9000u && full_path == "script-full-path" &&
                  function_name == "script-function",
              "stack preparation receives full path then function text");
        events.push_back("push-stack-module");
        return push_failed;
    }
    void ReleaseInterpreter(std::uint32_t handle) override {
        Check(handle == interpreter, "release uses interpreter handle");
        events.push_back("release-interpreter");
    }
    void ShowMissing(std::string_view message,
                     std::string_view title) override {
        events.push_back("message");
        missing_message = message;
        missing_title = title;
    }
    void CleanupScripts() override { events.push_back("cleanup-scripts"); }
    std::uint32_t ReleaseTemporary(Temp text,
                                   std::string_view value) override {
        switch (text) {
        case Temp::FunctionPath:
            Check(value == "PAL4-script:[world_hall][world_hall_init]",
                  "function path cleanup receives full stack key");
            events.push_back("release-function-path");
            return 11u;
        case Temp::ScriptPath:
            Check(value == "PAL4-script:[world_hall]",
                  "script path cleanup receives script key");
            events.push_back("release-script-path");
            return 22u;
        case Temp::FunctionName:
            Check(value == "world_hall_init",
                  "function name cleanup receives init name");
            events.push_back("release-function-name");
            return 33u;
        }
        throw std::runtime_error("unknown temporary text");
    }
};

void SetDefaults(Runtime& runtime) {
    runtime.texts[0x5008u] = "world";
    runtime.texts[0x50e0u] = "hall";
    runtime.words[0x50e4u] = 4u;
}

std::vector<std::string> Prefix() {
    return {
        "write-word:20716", "file-prefix", "read-text:20488",
        "mode", "store:20692", "read-text:20704",
        "read-word:20708", "store:20680", "read-text:20704",
        "read-word:20708", "initialize", "executor", "stack-module",
    };
}

std::vector<std::string> Suffix() {
    return {"cleanup-scripts", "release-function-path",
            "release-script-path", "release-function-name"};
}

void CheckPaths(const Runtime& runtime, const std::string_view extension) {
    Check(runtime.texts.at(0x50d4u) == "folder/world" +
                                            std::string(extension) &&
              runtime.texts.at(0x50c8u) == "PAL4-script:[world_hall]" &&
              runtime.module_key ==
                  "PAL4-script:[world_hall][world_hall_init]",
          "file, script and function paths match IDA concatenation");
}

void ActiveModule() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.module_active = true;
    auto expected = Prefix();
    const auto suffix = Suffix();
    expected.insert(expected.end(), suffix.begin(), suffix.end());
    Check(pal4::GameWorldSceneScriptLoader::Load(
              0x5000u, 0x1234u, runtime) == 33u &&
              runtime.events == expected && runtime.words[0x50ecu] == 0u &&
              runtime.mode_reads == 1u,
          "active module skips interpreter and returns final text cleanup result");
    CheckPaths(runtime, ".cs");
}

void ModuleAndPushBranches() {
    for (const bool failed : {true, false}) {
        Runtime runtime;
        SetDefaults(runtime);
        runtime.modes = {false, true};
        runtime.push_failed = failed;
        auto expected = Prefix();
        expected.insert(expected.end(), {
            "mode", "create", "executor", "register", "executor",
            "function-name", "full-path", "push-stack-module",
        });
        if (!failed) expected.push_back("write-word:20716");
        expected.push_back("release-interpreter");
        const auto suffix = Suffix();
        expected.insert(expected.end(), suffix.begin(), suffix.end());
        Check(pal4::GameWorldSceneScriptLoader::Load(
                  0x5000u, 0u, runtime) == 33u &&
                  runtime.events == expected &&
                  runtime.words[0x50ecu] == (failed ? 0u : 1u) &&
                  runtime.request.csb_mode &&
                  runtime.request.script_path ==
                      "PAL4-script:[world_hall]" &&
                  runtime.request.function_path ==
                      "PAL4-script:[world_hall][world_hall_init]" &&
                  runtime.request.function_name == "world_hall_init" &&
                  runtime.request.file_path == "folder/world.cs" &&
                  runtime.request.read_callback == 0x600d50u,
              "interpreter mode is reread and successful stack preparation sets queued flag");
    }
}

void MissingScriptBranches() {
    for (const bool created : {false, true}) {
        Runtime runtime;
        SetDefaults(runtime);
        runtime.modes = {true, true};
        runtime.interpreter = created ? 0x7000u : 0u;
        runtime.registration_succeeded = false;
        auto expected = Prefix();
        expected.push_back("mode");
        expected.push_back("create");
        if (created) {
            expected.push_back("executor");
            expected.push_back("register");
        }
        expected.push_back("message");
        const auto suffix = Suffix();
        expected.insert(expected.end(), suffix.begin(), suffix.end());
        Check(pal4::GameWorldSceneScriptLoader::Load(
                  0x5000u, 0x9999u, runtime) == 33u &&
                  runtime.events == expected &&
                  runtime.words[0x50ecu] == 0u &&
                  runtime.missing_message == "not found script files" &&
                  runtime.missing_title == "no script" &&
                  runtime.request.file_path == "folder/world.csb" &&
                  runtime.request.csb_mode,
              "create/register failure displays exact missing-script message");
        CheckPaths(runtime, ".csb");
    }
}

void CStringAndStub() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.module_active = true;
    runtime.texts[0x5008u] = std::string("world\0tail", 10);
    runtime.texts[0x50e0u] = "hallTRAIL";
    Check(pal4::GameWorldSceneScriptLoader::Load(
              0x5000u, 0u, runtime) == 33u,
          "world C string and explicit scene length are accepted");
    CheckPaths(runtime, ".cs");

    pal4::GameWorldSceneScriptRuntimeStub stub;
    Check(pal4::GameWorldSceneScriptLoader::Load(
              0x5000u, 0u, stub) == 0u,
          "isolated script stub links and completes active-module cleanup path");
}

}  // namespace

int main() {
    try {
        ActiveModule();
        ModuleAndPushBranches();
        MissingScriptBranches();
        CStringAndStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
