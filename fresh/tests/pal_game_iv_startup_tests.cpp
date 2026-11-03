#include "pal4/pal_game_iv_startup.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVStartupRuntime {
    std::vector<std::string> events;
    bool csb = false;
    bool cached = false;
    bool add_result = false;
    std::uint32_t interpreter = 0u;
    std::uint32_t pet_write_address = 0u;
    std::uint32_t pet_write_value = 1u;
    std::uint32_t create_calls = 0u;
    std::uint32_t add_calls = 0u;
    std::uint32_t script_instance_calls = 0u;

    std::uint32_t Inventory() override {
        events.push_back("inventory-get");
        return 0x1000u;
    }
    void InitializeInventory(std::uint32_t value) override {
        Check(value == 0x1000u, "inventory handle");
        events.push_back("inventory-init");
    }
    std::uint32_t PlayerManager() override {
        events.push_back("player-get");
        return 0x2000u;
    }
    void ProcessLoadedPlayers(std::uint32_t value) override {
        Check(value == 0x2000u, "player handle");
        events.push_back("player-process");
    }
    std::uint32_t ScriptInstance() override {
        ++script_instance_calls;
        events.push_back("script-get");
        return 0x3000u;
    }
    void ClearScriptCache(std::uint32_t value) override {
        Check(value == 0x3054u, "script cache clear offset");
        events.push_back("cache-clear");
    }
    std::uint32_t Camera() override {
        events.push_back("camera-get");
        return 0x4000u;
    }
    void ConfigureCamera(std::uint32_t value) override {
        Check(value == 0x4000u, "camera handle");
        events.push_back("camera-configure");
    }
    std::uint32_t ArenaCache() override {
        events.push_back("arena-get");
        return 0x5000u;
    }
    void ClearArenaCache(std::uint32_t value) override {
        Check(value == 0x5000u, "arena cache handle");
        events.push_back("arena-clear");
    }
    void InitializeCoreSystems(std::uint32_t value) override {
        Check(value == 0x6000u, "game object");
        events.push_back("core-init");
    }
    std::uint32_t GlobalData() override {
        events.push_back("global-get");
        return 0x7000u;
    }
    void SaveDefaultDialogSettings(std::uint32_t value) override {
        Check(value == 0x7000u, "global handle");
        events.push_back("dialog-defaults");
    }
    std::uint32_t PetManager() override {
        events.push_back("pet-get");
        return 0x8000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        pet_write_address = address;
        pet_write_value = value;
        events.push_back("pet-reset");
    }
    bool IsCsbMode() override {
        events.push_back("mode");
        return csb;
    }
    void InitializeScriptEngine() override {
        events.push_back("script-init");
    }
    bool ScriptCacheContains(std::uint32_t cache,
                             std::string_view full_name) override {
        Check(cache == 0x3054u && full_name == "PAL4-NewGame[NewGame]",
              "script cache query");
        events.push_back("cache-check");
        return cached;
    }
    std::uint32_t CreateInterpreter(
        bool csb_mode, std::string_view script_name,
        std::string_view full_name, std::string_view function_name,
        std::string_view source_path, std::uint32_t callback) override {
        ++create_calls;
        Check(csb_mode == csb && script_name == "PAL4-NewGame" &&
                  full_name == "PAL4-NewGame[NewGame]" &&
                  function_name == "NewGame" &&
                  source_path == (csb
                      ? "gamedata\\editData\\script\\script.csb"
                      : "gamedata\\editData\\script\\script.cs") &&
                  callback == 0x600d50u, "interpreter arguments");
        events.push_back("interpreter-create");
        return interpreter;
    }
    bool AddScriptModule(std::uint32_t cache,
                         std::uint32_t value) override {
        ++add_calls;
        Check(cache == 0x3054u && value == interpreter,
              "script-module registration arguments");
        events.push_back("module-add");
        return add_result;
    }
    std::string_view InterpreterName(std::uint32_t value) override {
        Check(value == interpreter, "script name interpreter");
        events.push_back("interpreter-name");
        return "NewGame";
    }
    std::string_view InterpreterFullPath(std::uint32_t value) override {
        Check(value == interpreter, "full path interpreter");
        events.push_back("interpreter-path");
        return "PAL4-NewGame[NewGame]";
    }
    void PushStackModule(std::uint32_t cache,
                         std::string_view full_path,
                         std::string_view name) override {
        Check(cache == 0x3054u &&
                  full_path == "PAL4-NewGame[NewGame]" &&
                  name == "NewGame", "script-stack module arguments");
        events.push_back("stack-push");
    }
    void ReleaseInterpreter(std::uint32_t value) override {
        Check(value == interpreter, "release registered interpreter");
        events.push_back("interpreter-release");
    }
    void ShowMissingScriptMessage(std::string_view message,
                                  std::string_view title) override {
        Check(message == "not found script files" && title == "no script",
              "missing script message");
        events.push_back("message-box");
    }
    void CleanupAllScripts() override {
        events.push_back("scripts-cleanup");
    }
};

std::vector<std::string> Prefix() {
    return {
        "inventory-get", "inventory-init", "player-get", "player-process",
        "script-get", "cache-clear", "camera-get", "camera-configure",
        "arena-get", "arena-clear", "core-init", "global-get",
        "dialog-defaults", "pet-get", "pet-reset", "mode", "script-init",
        "script-get", "cache-check",
    };
}

void CheckPath(bool csb, bool cached, std::uint32_t interpreter,
               bool add_result, const std::vector<std::string>& branch,
               std::uint32_t script_gets) {
    Runtime runtime;
    runtime.csb = csb;
    runtime.cached = cached;
    runtime.interpreter = interpreter;
    runtime.add_result = add_result;
    Check(pal4::PalGameIVStartup::Initialize(0x6000u, runtime) == 1u,
          "startup return");
    auto expected = Prefix();
    expected.insert(expected.end(), branch.begin(), branch.end());
    expected.push_back("scripts-cleanup");
    Check(runtime.events == expected, "startup call order");
    Check(runtime.pet_write_address == 0x8654u &&
              runtime.pet_write_value == 0u, "pet state reset");
    Check(runtime.script_instance_calls == script_gets &&
              runtime.create_calls == (cached ? 0u : 1u) &&
              runtime.add_calls == (interpreter != 0u && !cached ? 1u : 0u),
          "startup branch call counts");
}

}  // namespace

int main() {
    try {
        CheckPath(false, true, 0u, false, {}, 2u);
        CheckPath(true, false, 0u, false,
                  {"interpreter-create", "message-box"}, 2u);
        CheckPath(false, false, 0x9000u, false,
                  {"interpreter-create", "script-get", "module-add",
                   "message-box"}, 3u);
        CheckPath(true, false, 0x9000u, true,
                  {"interpreter-create", "script-get", "module-add",
                   "script-get", "interpreter-name", "interpreter-path",
                   "stack-push", "interpreter-release"}, 4u);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
