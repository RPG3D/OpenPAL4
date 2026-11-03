#include "pal4/script_combat_start.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCombatStartRuntime {
    pal4::ScriptCombatExecutor executor{0x101100u, {}};
    std::vector<std::string> calls;
    std::string throw_at;
    std::uint32_t destroyed_result = 0xcafeu;

    void MaybeThrow(std::string_view stage) {
        if (throw_at == stage) throw std::runtime_error(std::string(stage));
    }
    pal4::ScriptCombatExecutor& ScriptExecutor() override {
        calls.emplace_back("script");
        return executor;
    }
    void CreateDocument(std::string_view root) override {
        calls.emplace_back("create:" + std::string(root));
        MaybeThrow("create");
    }
    void SetCombatWorld(std::string_view key, std::string_view world,
                        std::uint32_t mode, std::uint32_t count) override {
        Check(key == "CombatWorld" && mode == 0u && count == 1u,
              "combat world property operands");
        calls.emplace_back("world:" + std::string(world));
        MaybeThrow("world");
    }
    void SetCombatMonster(std::uint32_t id, std::uint32_t value,
                          std::uint32_t group) override {
        Check(group == 0u, "monster group must be zero");
        calls.emplace_back("monster:" + std::to_string(id) + ":" +
                           std::to_string(value));
        MaybeThrow("monster");
    }
    void LogStartCombat(std::uint32_t source_line,
                        std::string_view placeholder,
                        std::uint32_t monster_count,
                        std::string_view world) override {
        Check(source_line == 0x339u && placeholder == "none",
              "logger source metadata");
        calls.emplace_back("log:" + std::to_string(monster_count) + ":" +
                           std::string(world));
        MaybeThrow("log");
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        MaybeThrow("game");
        return 0x104600u;
    }
    void EnterGameState(std::uint32_t game, std::uint32_t state) override {
        Check(game == 0x104600u && state == 3u,
              "PALIV_COMBAT state operands");
        calls.emplace_back("state");
        MaybeThrow("state");
    }
    std::uint32_t DestroyDocument() noexcept override {
        calls.emplace_back("destroy");
        return destroyed_result;
    }
};

void EmptyQueues() {
    Runtime unallocated;
    Check(pal4::ScriptCombatStarter::Start("world", unallocated) ==
              unallocated.executor.address &&
          unallocated.calls == std::vector<std::string>{"script"},
          "unallocated queue returns script executor without document");

    Runtime allocated;
    (void)allocated.executor.commands.Push(1u, 2u);
    allocated.executor.commands.Clear();
    const auto capacity = allocated.executor.commands.Capacity();
    Check(pal4::ScriptCombatStarter::Start("world", allocated) == 0u &&
          allocated.calls == std::vector<std::string>{"script"} &&
          allocated.executor.commands.Capacity() == capacity,
          "allocated empty queue returns zero and retains capacity");
}

void StartAndClear() {
    Runtime runtime;
    (void)runtime.executor.commands.Push(10u, 20u);
    (void)runtime.executor.commands.Push(30u, 40u);
    const auto capacity = runtime.executor.commands.Capacity();
    Check(pal4::ScriptCombatStarter::Start("arena", runtime) ==
              runtime.destroyed_result,
          "XML destructor result is returned");
    const std::vector<std::string> expected{
        "script", "create:CombatParam", "world:arena",
        "script", "script", "script", "monster:10:20",
        "script", "script", "script", "monster:30:40",
        "script", "script", "log:2:arena", "game", "state",
        "script", "destroy"};
    Check(runtime.calls == expected, "singleton re-fetch and effect order");
    Check(runtime.executor.commands.Commands().empty() &&
          runtime.executor.commands.Capacity() == capacity,
          "successful start clears records without freeing storage");
}

void NullWorldAndIgnoredThirdWord() {
    Runtime runtime;
    (void)runtime.executor.commands.Push(0xffffffffu, 0x80000000u);
    Check(pal4::ScriptCombatStarter::Start(nullptr, runtime) ==
              runtime.destroyed_result,
          "null world start result");
    Check(runtime.calls[2] == "world:" &&
          runtime.calls[6] == "monster:4294967295:2147483648" &&
          runtime.calls[9] == "log:1:",
          "null world falls back to empty and monster words stay unsigned");
}

void FailureCleanup() {
    for (const std::string stage : {
             "create", "world", "monster", "log", "game", "state"}) {
        Runtime runtime;
        (void)runtime.executor.commands.Push(1u, 2u);
        runtime.throw_at = stage;
        try {
            (void)pal4::ScriptCombatStarter::Start("world", runtime);
            throw std::runtime_error("missing failure");
        } catch (const std::runtime_error& error) {
            Check(error.what() == stage, "original exception propagates");
        }
        Check(runtime.calls.back() == (stage == "create" ?
                                        "create:CombatParam" : "destroy"),
              "constructed XML document is destroyed on failure");
        Check(runtime.executor.commands.Commands().size() == 1u,
              "failure leaves pending command available");
    }
}

}  // namespace

int main() {
    try {
        EmptyQueues();
        StartAndClear();
        NullWorldAndIgnoredThirdWord();
        FailureCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
