#include "pal4/game_combat_handoff.h"
#include "pal4/monster_behavior_manager.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::GameCombatHandoffRuntimeStub {
    std::uint32_t controller{0x4000u};
    std::int32_t ready{1};
    std::uint32_t state_begin{0x7000u};
    std::uint32_t state_end{0x7024u};
    std::string resolved_path{"Arena/scene"};
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> texts;
    std::vector<std::uint32_t> monster_words;
    std::int32_t destroy_result{57};
    std::int32_t world_count{-1};
    std::uint32_t world_mode{99u};
    std::string world_path;
    std::uint8_t mode_byte{};
    bool throw_on_world{};

    std::uint32_t StateController() override {
        calls.emplace_back("controller");
        return controller;
    }
    std::int32_t ControllerReady(std::uint32_t address) override {
        Check(address == controller, "controller identity");
        calls.emplace_back("ready");
        return ready;
    }
    void CreateDocument(std::string_view root) override {
        Check(root == "CombatParam", "combat document root");
        calls.emplace_back("document");
    }
    std::string ArenaName() override {
        calls.emplace_back("arena");
        return "Scene42";
    }
    std::string ResolveArenaPath(std::string_view arena,
                                 std::uint8_t mode) override {
        Check(arena == "Scene42", "arena source text");
        mode_byte = mode;
        calls.emplace_back("resolve");
        return resolved_path;
    }
    void ClearStateText() override { calls.emplace_back("clear-state-text"); }
    void AddStateText(std::uint32_t address) override {
        calls.emplace_back("add-text");
        texts.push_back(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t EmptyTextAddress() override { return 0x9999u; }
    void FinalizeManagerText(pal4::MonsterBehaviorManagerState& manager,
                             std::uint32_t begin,
                             std::uint32_t end) override {
        Check(begin == manager.words[0x724u / 4u] &&
                  end == manager.words[0x728u / 4u],
              "text array bounds");
        calls.emplace_back("finalize-text");
        manager.words[0x728u / 4u] = begin;
    }
    std::uint32_t StateMonsterBegin() override {
        calls.emplace_back("state-begin");
        return state_begin;
    }
    std::uint32_t StateMonsterEnd() override {
        calls.emplace_back("state-end");
        return state_end;
    }
    void SetCombatWorld(std::string_view node, std::string_view path,
                        std::uint32_t mode, std::int32_t count) override {
        Check(node == "combatWorld", "combat world node");
        calls.emplace_back("world");
        if (throw_on_world)
            throw std::runtime_error("world");
        world_mode = mode;
        world_count = count;
        world_path = path;
    }
    void SetCombatMonster(std::uint32_t id, std::uint32_t value,
                          std::uint32_t group) override {
        calls.emplace_back("monster");
        monster_words.insert(monster_words.end(), {id, value, group});
    }
    void InitializeCombatState(std::uint32_t state_id) override {
        Check(state_id == pal4::GameCombatHandoff::kCombatState,
              "PALIV_COMBAT state ID");
        calls.emplace_back("initialize");
    }
    std::int32_t DestroyDocument() override {
        calls.emplace_back("destroy");
        return destroy_result;
    }
};

pal4::MonsterBehaviorManagerState Manager() {
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x700u / 4u] = 1u;
    manager.words[0x724u / 4u] = 0x1000u;
    manager.words[0x728u / 4u] = 0x1020u;
    manager.words[0x714u / 4u] = 0x2000u;
    manager.words[0x718u / 4u] = 0x2018u;
    return manager;
}

void Gates() {
    auto manager = Manager();
    Runtime missing;
    missing.controller = 0u;
    Check(pal4::GameCombatHandoff::Process(manager, 2, missing) == 0 &&
              missing.calls == std::vector<std::string>{"controller"} &&
              manager.words[0x718u / 4u] == 0x2018u,
          "missing state controller has no side effects");
    Runtime inactive;
    inactive.ready = 7;
    Check(pal4::GameCombatHandoff::Process(manager, 2, inactive) == 7 &&
              inactive.calls == std::vector<std::string>{
                  "controller", "ready"} &&
              manager.words[0x700u / 4u] == 1u,
          "non-ready controller returns its raw result");
}

void FullHandoff() {
    auto manager = Manager();
    Runtime runtime;
    runtime.words[0x1004u] = 0x3000u;
    runtime.words[0x1014u] = 0u;
    runtime.words[0x2000u] = 11u;
    runtime.words[0x2004u] = 22u;
    runtime.words[0x2008u] = 2u;
    runtime.words[0x200cu] = 33u;
    runtime.words[0x2010u] = 44u;
    runtime.words[0x2014u] = 3u;
    Check(pal4::GameCombatHandoff::Process(manager, 2, runtime) == 57 &&
              manager.words[0x700u / 4u] == 2u &&
              manager.words[0x728u / 4u] == 0x1000u &&
              manager.words[0x718u / 4u] == 0x2000u &&
              runtime.texts == std::vector<std::uint32_t>{
                  0x3000u, 0x9999u} &&
              runtime.world_mode == 1u && runtime.world_count == 3 &&
              runtime.world_path == "Arena/scene" &&
              runtime.monster_words == std::vector<std::uint32_t>{
                  11u, 22u, 1u, 33u, 44u, 0u} &&
              runtime.mode_byte == 2u &&
              runtime.calls == std::vector<std::string>{
                  "controller", "ready", "document", "arena", "resolve",
                  "clear-state-text", "add-text", "add-text",
                  "finalize-text", "state-begin", "state-end", "world",
                  "monster", "monster", "initialize", "destroy"},
          "combat handoff orders text, world, monsters and state transition");
}

void ModesAndFallback() {
    for (const auto [mode, expected_world] : {
             std::pair{0, 0u}, std::pair{1, 2u}, std::pair{2, 1u}}) {
        auto manager = Manager();
        manager.words[0x724u / 4u] = 0u;
        manager.words[0x728u / 4u] = 0u;
        manager.words[0x714u / 4u] = 0u;
        manager.words[0x718u / 4u] = 0u;
        Runtime runtime;
        runtime.state_begin = 0u;
        runtime.resolved_path.clear();
        Check(pal4::GameCombatHandoff::Process(manager, mode, runtime) ==
                  57 &&
                  runtime.world_mode == expected_world &&
                  runtime.world_count == 0 && runtime.world_path == "x" &&
                  runtime.texts.empty() && runtime.monster_words.empty(),
              "all supported combat modes and empty arena fallback");
    }
    auto manager = Manager();
    manager.words[0x700u / 4u] = 4u;
    manager.words[0x724u / 4u] = manager.words[0x728u / 4u];
    manager.words[0x714u / 4u] = manager.words[0x718u / 4u];
    Runtime runtime;
    Check(pal4::GameCombatHandoff::Process(manager, -1, runtime) == 57 &&
              manager.words[0x700u / 4u] == 4u &&
              runtime.world_mode == 99u &&
              runtime.mode_byte == 0xffu,
          "negative request retains unsupported stored mode");
}

void LinkableStub() {
    pal4::MonsterBehaviorManagerState manager{};
    pal4::GameCombatHandoffRuntimeStub runtime;
    Check(pal4::GameCombatHandoff::Process(manager, 0, runtime) == 0,
          "combat handoff links without original game services");
}

void StubComposesTextErase() {
    struct ReadyStub final : pal4::GameCombatHandoffRuntimeStub {
        std::uint32_t StateController() override { return 1u; }
        std::int32_t ControllerReady(std::uint32_t) override { return 1; }
    } runtime;
    pal4::MonsterBehaviorManagerState manager{};
    manager.words[0x724u / 4u] = 0x1000u;
    manager.words[0x728u / 4u] = 0x1020u;
    Check(pal4::GameCombatHandoff::Process(manager, 0, runtime) == 0 &&
              manager.words[0x728u / 4u] == 0x1000u,
          "default combat runtime composes reconstructed text-array erase");
}

void ExceptionDestroysDocument() {
    auto manager = Manager();
    manager.words[0x724u / 4u] = manager.words[0x728u / 4u];
    manager.words[0x714u / 4u] = manager.words[0x718u / 4u];
    Runtime runtime;
    runtime.throw_on_world = true;
    bool threw = false;
    try {
        (void)pal4::GameCombatHandoff::Process(manager, 0, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && !runtime.calls.empty() &&
              runtime.calls.back() == "destroy" &&
              manager.words[0x718u / 4u] == 0x2018u,
          "world construction failure destroys document before unwinding");
}

}  // namespace

int main() {
    Gates();
    FullHandoff();
    ModesAndFallback();
    LinkableStub();
    StubComposesTextErase();
    ExceptionDestroysDocument();
    std::cout << "game combat handoff cases passed\n";
}
