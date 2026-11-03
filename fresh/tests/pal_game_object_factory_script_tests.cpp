#include "pal4/pal_game_object_factory_script.h"

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

struct Runtime final : pal4::IPalGameObjectFactoryScriptRuntime {
    static constexpr std::uint32_t handle = 0x1000u;
    static constexpr std::uint32_t object = 0x2000u;
    static constexpr std::uint32_t arena = 0x3000u;
    static constexpr std::uint32_t movement = 0x4000u;
    static constexpr std::uint32_t movement_root = 0x4100u;
    static constexpr std::uint32_t interpreter = 0x7000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::string key;
    std::string path;
    std::string function;
    std::string pushed_path;
    std::string pushed_function;
    std::uint32_t context = 0u;
    std::uint32_t interpreter_result = interpreter;
    bool cache_hit = false;
    bool add_result = true;
    std::int32_t push_result = 1;
    bool csb_mode = true;

    Runtime() {
        words[handle + 4u] = object;
        words[object + 48u] = 0x5000u;
        words[object + 52u] = 5u;
        strings[0x5000u] = "FightX";
        words[arena + 200u] = 0x6000u;
        words[arena + 212u] = 0x7777u;
        strings[0x6000u] = "assets/";
        words[movement] = movement_root;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("movement flag");
        words[address] = value;
    }
    std::string ReadBytes(std::uint32_t address,
                          std::uint32_t size) override {
        if (size == 0u) return {};
        return strings.at(address).substr(0u, size);
    }
    std::string ReadCString(std::uint32_t address) override {
        return strings.at(address);
    }
    std::uint32_t ArenaManager() override { return arena; }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("movement adapter");
        return movement;
    }
    void InitializeScript() override {
        calls.emplace_back("script init");
    }
    bool IsCsbMode() override { return csb_mode; }
    bool ScriptCacheContains(std::string_view incoming) override {
        calls.emplace_back("cache");
        key = std::string(incoming);
        return cache_hit;
    }
    std::uint32_t CreateScriptInterpreter(
        bool incoming_csb, std::string_view incoming_path,
        std::string_view incoming_key,
        std::string_view incoming_function,
        std::uint32_t incoming_context) override {
        Check(incoming_csb == csb_mode && incoming_key == key,
              "script interpreter mode and cache key");
        calls.emplace_back("create interpreter");
        path = std::string(incoming_path);
        function = std::string(incoming_function);
        context = incoming_context;
        return interpreter_result;
    }
    bool AddScriptModule(
        std::uint32_t incoming) override {
        Check(incoming == interpreter, "registered interpreter address");
        calls.emplace_back("add module");
        return add_result;
    }
    std::string ScriptFunctionName(std::uint32_t incoming) override {
        Check(incoming == interpreter, "function-name interpreter");
        return "OnFight";
    }
    std::string ScriptFullPath(std::uint32_t incoming) override {
        Check(incoming == interpreter, "full-path interpreter");
        return "assets/Q01";
    }
    std::int32_t PushStackModule(std::string_view path,
                               std::string_view name) override {
        calls.emplace_back("push stack");
        pushed_path = std::string(path);
        pushed_function = std::string(name);
        return push_result;
    }
    void ReleaseInterpreter(std::uint32_t incoming) override {
        Check(incoming == interpreter, "released interpreter address");
        calls.emplace_back("release");
    }
    void ShowMissingScriptMessage() override {
        calls.emplace_back("missing script");
    }
    void CleanupAllScripts() override {
        calls.emplace_back("script cleanup");
    }
    std::uint32_t Floor() {
        return pal4::PalGameObjectFactoryScript::ExecuteLocalFloor(
            handle, 0u, *this);
    }
    std::uint32_t Machine() {
        return pal4::PalGameObjectFactoryScript::ExecuteMachine(
            handle, 0u, *this);
    }
};

void CheckGates() {
    Runtime argument;
    Check(pal4::PalGameObjectFactoryScript::ExecuteLocalFloor(
              Runtime::handle, 1u, argument) == 0u &&
          pal4::PalGameObjectFactoryScript::ExecuteMachine(
              Runtime::handle, 1u, argument) == 0u &&
          argument.calls.empty(),
          "nonzero argument gates both callbacks");

    Runtime no_object;
    no_object.words[Runtime::handle + 4u] = 0u;
    Check(no_object.Floor() == 0u && no_object.Machine() == 0u &&
          no_object.calls.empty(),
          "null object gates both callbacks");

    Runtime empty_machine;
    empty_machine.words[Runtime::object + 52u] = 0u;
    Check(empty_machine.Machine() == 0u && empty_machine.calls.empty(),
          "machine rejects empty script name");
    empty_machine.words[Runtime::object + 52u] = 0xffffffffu;
    Check(empty_machine.Machine() == 0u && empty_machine.calls.empty(),
          "machine rejects negative script length");

    Runtime empty_floor;
    empty_floor.words[Runtime::object + 52u] = 0u;
    empty_floor.cache_hit = true;
    Check(empty_floor.Floor() == 0u &&
          empty_floor.key == "assets/[]" &&
          empty_floor.calls == (std::vector<std::string>{
              "script init", "cache", "script cleanup"}),
          "local-floor callback permits empty script name");
}

void CheckScriptPaths() {
    Runtime cached;
    cached.cache_hit = true;
    Check(cached.Floor() == 0u && cached.key == "assets/[Fight]" &&
          cached.calls == (std::vector<std::string>{
              "script init", "cache", "script cleanup"}),
          "cached local-floor script needs no interpreter");
    Runtime cached_machine;
    cached_machine.cache_hit = true;
    Check(cached_machine.Machine() == 0u &&
          cached_machine.key == "assets/[Fight]" &&
          cached_machine.calls == cached.calls,
          "cached machine script uses same path");

    Runtime missing;
    missing.interpreter_result = 0u;
    Check(missing.Floor() == 0u &&
          missing.path == "assets/" && missing.function == "Fight" &&
          missing.context == 0x7777u &&
          missing.calls == (std::vector<std::string>{
              "script init", "cache", "create interpreter",
              "missing script", "script cleanup"}),
          "missing interpreter reports and cleans script packages");

    Runtime rejected;
    rejected.add_result = false;
    Check(rejected.Machine() == 0u &&
          rejected.calls == (std::vector<std::string>{
              "script init", "cache", "create interpreter", "add module",
              "missing script", "script cleanup"}),
          "rejected interpreter follows missing-script path");

    Runtime handled;
    Check(handled.Floor() == 0u &&
          handled.pushed_path == "assets/Q01" &&
          handled.pushed_function == "OnFight" &&
          handled.calls == (std::vector<std::string>{
              "script init", "cache", "create interpreter", "add module",
              "push stack", "release", "script cleanup"}),
          "failed stack push returns zero and releases interpreter");
}

void CheckSuccessfulPush() {
    Runtime floor;
    floor.push_result = false;
    Check(floor.Floor() == 1u &&
          floor.words.at(Runtime::movement_root + 88u) == 1u &&
          floor.calls == (std::vector<std::string>{
              "script init", "cache", "create interpreter", "add module",
              "push stack", "movement adapter", "movement flag", "release",
              "script cleanup"}),
          "successful local-floor push marks collision stop before release");

    Runtime missing_function;
    missing_function.push_result=-6;
    Check(missing_function.Floor()==0u && missing_function.words.find(Runtime::movement_root+88u)==missing_function.words.end(),
          "missing function does not claim successful stack push");
    Runtime machine;
    machine.push_result = false;
    Check(machine.Machine() == 1u &&
          machine.words.find(Runtime::movement_root + 88u) ==
              machine.words.end() &&
          machine.calls == (std::vector<std::string>{
              "script init", "cache", "create interpreter", "add module",
              "push stack", "release", "script cleanup"}),
          "successful machine push does not set collision stop");
}

}  // namespace

int main() {
    try {
        CheckGates();
        CheckScriptPaths();
        CheckSuccessfulPush();
        std::cout << "pal_game_object_factory_script_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
