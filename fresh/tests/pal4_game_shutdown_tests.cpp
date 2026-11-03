#include "pal4/pal4_game_shutdown.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPal4GameShutdownRuntime {
    std::vector<std::string> calls;
    std::uint32_t game_reads = 0;
    std::uint32_t game_queries = 0;
    std::uint32_t first_object = 0x3000;
    std::uint32_t second_object = 0x3000;
    std::uint32_t combat_object = 0x7000;
    std::int32_t shutdown_result = 0;

    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x1000 + ++game_queries * 0x1000;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x218c || address == 0x318c) {
            calls.emplace_back("game object");
            return ++game_reads == 1 ? first_object : second_object;
        }
        Check(address == 0x61a8, "combat object offset");
        calls.emplace_back("combat object");
        return combat_object;
    }
    std::int32_t ShutdownGameObject(std::uint32_t object) override {
        Check(object == second_object,
              "second game instance supplies shutdown object");
        calls.emplace_back("shutdown");
        return shutdown_result;
    }
    std::uint32_t FontManager() override {
        calls.emplace_back("font manager");
        return 0x4000;
    }
    void GetFontTextures(std::uint32_t manager) override {
        Check(manager == 0x4000, "font texture manager");
        calls.emplace_back("font textures");
    }
    void GetFontTextureStates(std::uint32_t manager) override {
        Check(manager == 0x4000, "font texture state manager");
        calls.emplace_back("font states");
    }
    void GetEffectScriptSlot284() override {
        calls.emplace_back("script slot 284");
    }
    void GetEffectScriptSlot288() override {
        calls.emplace_back("script slot 288");
    }
    std::uint32_t ArenaEffectManager() override {
        calls.emplace_back("effects");
        return 0x5000;
    }
    std::uint32_t CleanupCollection(std::uint32_t collection) override {
        Check(collection == 0x5104 || collection == 0x7104,
              "ordinary collection offset");
        calls.emplace_back(collection == 0x5104 ? "clean effects" :
                                                  "clean combat");
        return collection == 0x5104 ? 0xaaaa : 0xbbbb;
    }
    std::uint32_t CleanupCollectionAlternate(
        std::uint32_t collection) override {
        Check(collection == 0x5104 || collection == 0x7104,
              "alternate collection offset");
        calls.emplace_back(collection == 0x5104 ? "clean effects alt" :
                                                  "clean combat alt");
        return collection == 0x5104 ? 0xcccc : 0xdddd;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat");
        return 0x6000;
    }
    std::uint32_t ScriptState() override {
        calls.emplace_back("script state");
        return 0x1111;
    }
    std::uint32_t ScriptProperty288() override {
        calls.emplace_back("script property 288");
        return 0x2222;
    }
};

void TestShutdownIfRequested() {
    Runtime runtime;
    Check(pal4::Pal4GameShutdown::ShutdownIfRequested(0, runtime) == 2 &&
              runtime.calls.empty(), "zero request avoids singleton access");
    runtime.first_object = 0;
    Check(pal4::Pal4GameShutdown::ShutdownIfRequested(1, runtime) == 2 &&
              runtime.calls == std::vector<std::string>{"game", "game object"},
          "missing game object returns two");
    runtime = Runtime{};
    runtime.second_object = 0x3100;
    runtime.shutdown_result = -17;
    Check(pal4::Pal4GameShutdown::ShutdownIfRequested(-1, runtime) == 2 &&
              runtime.calls == std::vector<std::string>{
                                   "game", "game object", "game",
                                   "game object", "shutdown"},
          "requested shutdown reacquires game and ignores native return");
}

void TestCheckAndShutdown() {
    Runtime runtime;
    runtime.first_object = 0;
    Check(pal4::Pal4GameShutdown::CheckAndShutdown(runtime) == 2 &&
              runtime.calls == std::vector<std::string>{"game", "game object"},
          "absent object returns two");
    for (const auto value : {0, 1, -1, 2, 3}) {
        runtime = Runtime{};
        runtime.shutdown_result = value;
        Check(pal4::Pal4GameShutdown::CheckAndShutdown(runtime) ==
                  (value == 0 || value == 1 ? 1 : 2),
              "shutdown result maps only zero and one to one");
        Check(runtime.calls == std::vector<std::string>{
                                   "game", "game object", "game",
                                   "game object", "shutdown"},
              "check shutdown reacquires object before virtual call");
    }
}

void TestResourceCleanup() {
    Runtime runtime;
    Check(pal4::Pal4GameShutdown::CleanupResources(runtime) == 0xbbbb &&
              runtime.calls == std::vector<std::string>{
                                   "font manager", "font textures",
                                   "script slot 284", "effects",
                                   "clean effects", "combat", "script state",
                                   "combat object", "clean combat"},
          "ordinary cleanup order and combat collection result");
    runtime = Runtime{};
    runtime.combat_object = 0;
    Check(pal4::Pal4GameShutdown::CleanupResources(runtime) == 0x1111 &&
              runtime.calls == std::vector<std::string>{
                                   "font manager", "font textures",
                                   "script slot 284", "effects",
                                   "clean effects", "combat", "script state",
                                   "combat object"},
          "ordinary cleanup returns script state when combat object absent");
}

void TestAlternateCleanup() {
    Runtime runtime;
    Check(pal4::Pal4GameShutdown::CleanupAlternateResources(runtime) ==
                  0xdddd &&
              runtime.calls == std::vector<std::string>{
                                   "font manager", "font states",
                                   "script slot 288", "effects",
                                   "clean effects alt", "combat",
                                   "script property 288", "combat object",
                                   "clean combat alt"},
          "alternate cleanup order and combat collection result");
    runtime = Runtime{};
    runtime.combat_object = 0;
    Check(pal4::Pal4GameShutdown::CleanupAlternateResources(runtime) ==
                  0x2222 &&
              runtime.calls == std::vector<std::string>{
                                   "font manager", "font states",
                                   "script slot 288", "effects",
                                   "clean effects alt", "combat",
                                   "script property 288", "combat object"},
          "alternate cleanup returns script property when combat absent");
}

}  // namespace

int main() {
    try {
        TestShutdownIfRequested();
        TestCheckAndShutdown();
        TestResourceCleanup();
        TestAlternateCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
