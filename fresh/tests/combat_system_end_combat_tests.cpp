#include "pal4/combat_system_end_combat.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemEndRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void DispatchActorEnd(std::uint32_t object) override {
        Check(object == kObject, "actor end dispatch object");
        calls.emplace_back("dispatch");
    }
    void DeinitializeGameObject(std::uint32_t object) override {
        Check(object == 0x2000u, "game object pointer +424");
        calls.emplace_back("deinit");
    }
    std::uint32_t GetActorManager() override {
        calls.emplace_back("get-manager");
        return 0x3000u;
    }
    void DestructActorManager(std::uint32_t manager) override {
        Check(manager == 0x3000u, "actor manager cleanup target");
        calls.emplace_back("destruct-manager");
    }
    std::uint32_t GetConsole() override {
        calls.emplace_back("get-console");
        return 0x4000u;
    }
    void ResetConsole(std::uint32_t console) override {
        Check(console == 0x4000u, "console cleanup target");
        calls.emplace_back("reset-console");
    }
    void SetRenderCallback(std::uint32_t callback) override {
        Check(callback == 0x5000u, "saved render callback +428");
        calls.emplace_back("restore-render");
    }
    void InitializeStrings(std::uint32_t block) override {
        Check(block == kObject + 432u, "property strings block +432");
        calls.emplace_back("init-strings");
    }
};

void Cleanup() {
    Runtime runtime;
    runtime.words[kObject + 424u] = 0x2000u;
    runtime.words[kObject + 428u] = 0x5000u;
    runtime.words[kObject + 388u] = 4u;
    runtime.words[kObject + 392u] = 5u;
    Check(pal4::CombatSystemEndCombat::EndWrapper(
              kObject, runtime) == 1u &&
              runtime.words[kObject + 388u] == 0u &&
              runtime.words[kObject + 392u] == 0u &&
              runtime.words[kObject + 508u] == 7u &&
              runtime.bytes[kObject + 516u] == 1u,
          "end wrapper forwards result and state writes");
    Check(runtime.calls == std::vector<std::string>{
              "word:4484", "word:4488", "word:4604", "dispatch",
              "deinit", "get-manager", "destruct-manager",
              "get-console", "reset-console", "restore-render",
              "byte:4612", "init-strings"},
          "end cleanup follows IDA order");
    runtime.calls.clear();
    Check(pal4::CombatSystemEndCombat::End(kObject, runtime) == 1u &&
              runtime.calls.size() == 12u,
          "direct entry repeats cleanup and returns one");
}

}  // namespace

int main() {
    try {
        Cleanup();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
