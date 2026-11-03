#include "pal4/combat_system_ui.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemUiRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t frame = 0x2000u;

    Runtime() { words[0x2000u + 36u] = 0x3456u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void ProcessUiEvent(std::string_view name, std::uint32_t enabled,
                        std::uint32_t parameter) override {
        calls.emplace_back("ui:" + std::string(name) + ":" +
                           std::to_string(enabled) + ":" +
                           std::to_string(parameter));
    }
    std::uint32_t FindFrame(std::string_view name) override {
        Check(name == "CombatMainWindow", "active UI frame name");
        calls.emplace_back("find-frame");
        return frame;
    }
    std::uint32_t CreateHintWindow(std::uint32_t variant) override {
        Check(variant == 1u || variant == 2u, "hint variant");
        calls.emplace_back("create-hint:" + std::to_string(variant));
        return 0x3000u + variant;
    }
    void CleanupHintWindow(std::uint32_t window) override {
        Check(window == 0x3001u || window == 0x3002u,
              "hint cleanup receives created window");
        calls.emplace_back("cleanup-hint:" +
                           std::to_string(window - 0x3000u));
    }
    std::uint32_t GetConsole() override {
        calls.emplace_back("get-console");
        return 0x4000u;
    }
    void UnsubscribeConsoleEvents(std::uint32_t console) override {
        Check(console == 0x4000u, "console unsubscribe target");
        calls.emplace_back("unsubscribe");
    }
    std::uint32_t GetActorManager() override {
        calls.emplace_back("get-manager");
        return 0x5000u;
    }
    void DispatchActorEvent36(std::uint32_t collection) override {
        Check(collection == 0x5020u, "event36 actor collection offset");
        calls.emplace_back("actor36");
    }
    void EndCombat(std::uint32_t object) override {
        Check(object == kObject, "end-combat object");
        calls.emplace_back("end-combat");
    }
    std::uint32_t DispatchScriptEvent(std::string_view name,
                                      std::uint32_t payload) override {
        calls.emplace_back("script:" + std::string(name) + ":" +
                           std::to_string(payload));
        return 0x7777u;
    }
};

void ShowBranches() {
    Runtime runtime;
    Check(pal4::CombatSystemUi::ShowWrapper(kObject, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui:show_combat_ending:1:0",
                  "ui:show_combat_ending:0:0",
                  "ui:show_combat_main_ui:1:0", "find-frame",
                  "ui:show_combat_action_console:1:13398",
                  "ui:show_combat_role_state:1:13398"},
          "active show toggles ending and sends frame parameter");
    runtime.calls.clear();
    runtime.frame = 0u;
    Check(pal4::CombatSystemUi::Show(kObject, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui:show_combat_ending:1:0",
                  "ui:show_combat_ending:0:0",
                  "ui:show_combat_main_ui:1:0", "find-frame"},
          "missing main frame skips action and role events");
    runtime.calls.clear();
    runtime.bytes[kObject + 516u] = 1u;
    Check(pal4::CombatSystemUi::Show(kObject, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui:show_quick_combat_result:1:0",
                  "ui:show_quick_combat_result:0:0",
                  "ui:show_quick_combat:1:0"},
          "inactive show uses quick combat events only");
}

void CloseBranches() {
    Runtime runtime;
    Check(pal4::CombatSystemUi::CloseWrapper(kObject, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui:show_combat_main_ui:0:0",
                  "ui:show_combat_action_console:0:0",
                  "ui:show_combat_role_state:0:0",
                  "ui:show_combat_ending:0:0", "create-hint:1",
                  "cleanup-hint:1", "create-hint:2", "cleanup-hint:2"},
          "active close clears four events then both hint windows");
    runtime.calls.clear();
    runtime.bytes[kObject + 516u] = 0xffu;
    Check(pal4::CombatSystemUi::Close(kObject, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui:show_quick_combat:0:0",
                  "ui:show_quick_combat_result:0:0",
                  "create-hint:1", "cleanup-hint:1", "create-hint:2",
                  "cleanup-hint:2"},
          "inactive close clears quick UI before hints");
}

void EndingAndSequence() {
    Runtime runtime;
    Check(pal4::CombatSystemUi::ShowEndingWrapper(
              kObject, 0x6000u, runtime) == 1u &&
              runtime.words[kObject + 508u] == 6u &&
              runtime.calls == std::vector<std::string>{
                  "write:4604:6", "get-console", "unsubscribe",
                  "ui:show_combat_main_ui:0:0",
                  "ui:show_combat_action_console:0:0",
                  "ui:show_combat_role_state:0:0",
                  "ui:show_combat_ending:0:0", "create-hint:1",
                  "cleanup-hint:1", "create-hint:2", "cleanup-hint:2",
                  "ui:show_combat_ending:1:0", "script:combat_prize:24576"},
          "ending wrapper moves to state six, closes UI and dispatches prize");
    runtime.calls.clear();
    Check(pal4::CombatSystemUi::EndSequence(
              kObject, 0x1234u, runtime) == 0x7777u &&
              runtime.words[kObject + 508u] == 7u &&
              runtime.words[kObject + 504u] == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "ui:show_combat_ending:0:0", "get-manager", "actor36",
                  "write:4604:7", "write:4600:4660",
                  "ui:show_combat_main_ui:0:0",
                  "ui:show_combat_action_console:0:0",
                  "ui:show_combat_role_state:0:0",
                  "ui:show_combat_ending:0:0", "create-hint:1",
                  "cleanup-hint:1", "create-hint:2", "cleanup-hint:2",
                  "end-combat", "script:close_system:4660"},
          "end sequence closes ending UI, dispatches actor36 and close event");
}

}  // namespace

int main() {
    try {
        ShowBranches();
        CloseBranches();
        EndingAndSequence();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
