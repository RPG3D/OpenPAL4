#include "pal4/monster_combat_parameter_callback.h"

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

struct Runtime final : pal4::IMonsterCombatParameterCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
};

struct ProcessRuntime final : pal4::IMonsterCombatParameterProcessRuntime {
    static constexpr std::uint32_t game = 0x1000u;
    static constexpr std::uint32_t state = 0x1100u;
    static constexpr std::uint32_t callback = 0x2000u;
    static constexpr std::uint32_t monster = 0x3000u;
    static constexpr std::uint32_t actor = 0x4000u;
    static constexpr std::uint32_t hero = 0x5000u;
    static constexpr std::uint32_t movement = 0x6000u;
    static constexpr std::uint32_t movement_root = 0x6100u;
    static constexpr std::uint32_t arena = 0x7000u;
    static constexpr std::uint32_t monster_clump = 0x8000u;
    static constexpr std::uint32_t hero_clump = 0x8100u;
    static constexpr std::uint32_t monster_object = 0x9000u;
    static constexpr std::uint32_t hero_object = 0x9100u;
    static constexpr std::uint32_t monster_transform = 0xa000u;
    static constexpr std::uint32_t hero_transform = 0xa100u;
    static constexpr std::uint32_t behavior = 0xb000u;
    static constexpr std::uint32_t behavior_xml = 0xc000u;
    static constexpr std::uint32_t document = 0xe000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> events;
    std::string world_path = "world/audio";
    std::string script_name;
    std::string received_cache_key;
    std::string received_script_path;
    std::string received_script_name;
    std::string reported_message;
    std::string reported_function;
    std::uint32_t received_context = 0u;
    std::uint32_t world_mode = 0xffffffffu;
    std::uint32_t world_count = 0u;
    std::uint32_t combat_mode = 0xffffffffu;
    std::uint32_t dispatch_bits = 0u;
    std::uint32_t state_result = 1u;
    std::uint32_t interpreter = 0x1230u;
    double angle = 15.0 / 57.29578;
    float passed_dot = 0.0f;
    bool cache_hit = false;
    bool handle_result = true;
    bool csb_mode = false;
    bool throw_interpreter = false;

    ProcessRuntime() {
        // IDA5A9564/5A9575 uses +18C, not the adjacent state-list member.
        words[game + 392u] = 0xdeadbeefu;
        words[game + 396u] = state;
        words[callback + 8u] = monster;
        words[actor + 4u] = hero;
        words[actor + 732u] = 0u;
        words[actor + 736u] = 0u;
        words[movement] = movement_root;
        words[arena + 8u] = 0xd000u;
        words[arena + 200u] = 0xd100u;
        words[arena + 212u] = 0x7777u;
        strings[0xd000u] = "arena";
        strings[0xd100u] = "scripts/";
        words[monster + 20u] = monster_clump;
        words[hero + 20u] = hero_clump;
        words[monster_clump + 4u] = monster_object;
        words[hero_clump + 4u] = hero_object;
        words[monster + 1620u] = 0u;
        floats[monster_transform + 32u] = 1.0f;
        floats[monster_transform + 36u] = 0.0f;
        floats[monster_transform + 40u] = 0.0f;
        floats[hero_transform + 32u] = 0.0f;
        floats[hero_transform + 36u] = 1.0f;
        floats[hero_transform + 40u] = 0.0f;
        floats[monster + 172u] = 0.0f;
        floats[monster + 176u] = 1.0f;
        floats[monster + 180u] = 0.0f;
        floats[hero + 172u] = 0.0f;
        floats[hero + 176u] = 0.0f;
        floats[hero + 180u] = 0.0f;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == movement_root + 88u)
            events.emplace_back("flag");
        words[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    std::uint32_t GameInstance() override { return game; }
    std::uint32_t ActorControl() override { return actor; }
    std::uint32_t MovementAdapter() override { return movement; }
    std::uint32_t ArenaManager() override { return arena; }
    std::uint32_t InvokeGameStateSlot20(
        std::uint32_t incoming) override {
        Check(incoming == state, "game-state slot 20 target");
        return state_result;
    }
    void ClearGameState(std::uint32_t incoming) override {
        Check(incoming == game, "clear game-state target");
        events.emplace_back("clear");
    }
    std::uint32_t CreateCombatDocument(
        std::string_view root) override {
        Check(root == "CombatParam", "combat document root");
        events.emplace_back("create document");
        return document;
    }
    void DestroyCombatDocument(std::uint32_t incoming) override {
        Check(incoming == document, "combat document cleanup target");
        events.emplace_back("destroy document");
    }
    std::string ReadCString(std::uint32_t pointer) override {
        return strings.at(pointer);
    }
    std::string FloorSoundPath(std::string_view base) override {
        Check(base == "arena", "floor sound path input");
        return world_path;
    }
    std::uint32_t ObjectTransform(std::uint32_t object) override {
        if (object == monster_object) return monster_transform;
        Check(object == hero_object, "hero transform object");
        return hero_transform;
    }
    pal4::MonsterCombatVector3 NormalizeDirection(
        pal4::MonsterCombatVector3 direction) override {
        Check(direction.x == 0.0f && direction.z == 0.0f &&
              (direction.y == 1.0f || direction.y == -1.0f),
              "monster-to-hero direction");
        return direction;
    }
    double AngleFromDot(float dot) override {
        passed_dot = dot;
        return angle;
    }
    void SetCombatWorld(std::uint32_t incoming_document,
                        std::string_view property, std::string_view path,
                        std::uint32_t mode,
                        std::uint32_t count) override {
        Check(incoming_document == document && property == "combatWorld",
              "combat world document and key");
        events.emplace_back("world");
        world_path = std::string(path);
        world_mode = mode;
        world_count = count;
    }
    std::string ReadBehaviorScriptName(
        std::uint32_t node, std::string_view attribute) override {
        Check(node == behavior_xml &&
              attribute == "AAB_MST_ESSBEH_script_func",
              "behavior script attribute");
        events.emplace_back("attribute");
        return script_name;
    }
    void InitializeScript() override { events.emplace_back("script init"); }
    bool IsCsbMode() override { return csb_mode; }
    bool ScriptCacheContains(std::string_view key) override {
        received_cache_key = std::string(key);
        events.emplace_back("cache");
        return cache_hit;
    }
    std::uint32_t CreateScriptInterpreter(
        bool incoming_csb, std::string_view path,
        std::string_view cache_key, std::string_view function,
        std::uint32_t context) override {
        Check(incoming_csb == csb_mode &&
              cache_key == received_cache_key,
              "script interpreter flags and cache key");
        events.emplace_back("interpreter");
        received_script_path = std::string(path);
        received_script_name = std::string(function);
        received_context = context;
        if (throw_interpreter) throw std::runtime_error("interpreter");
        return interpreter;
    }
    bool LoadScriptModule(
        std::uint32_t incoming) override {
        Check(incoming == interpreter, "script exception interpreter");
        events.emplace_back("handle");
        return handle_result;
    }
    std::string ScriptFunctionName(std::uint32_t incoming) override {
        Check(incoming == interpreter, "script function interpreter");
        return "combat_start";
    }
    std::string ScriptModulePath(std::uint32_t incoming) override {
        Check(incoming == interpreter, "script error interpreter");
        return "scripts/[BeginFight]";
    }
    void PushScriptModule(std::string_view message,
                               std::string_view function) override {
        events.emplace_back("report");
        reported_message = std::string(message);
        reported_function = std::string(function);
    }
    void ReleaseInterpreter(std::uint32_t incoming) override {
        Check(incoming == interpreter, "released interpreter");
        events.emplace_back("release");
    }
    void ShowMissingScriptMessage() override {
        events.emplace_back("missing script");
    }
    void CleanupAllScripts() override {
        events.emplace_back("script cleanup");
    }
    void AddActiveCharacter(std::uint32_t incoming_arena,
                            std::uint32_t incoming_monster,
                            std::uint32_t flag) override {
        Check(incoming_arena == arena && incoming_monster == monster &&
              flag == 0x40000000u,
              "active character arguments");
        events.emplace_back("active character");
    }
    void SetClumpRenderCallback(std::uint32_t adapter_root,
                                std::uint32_t clump) override {
        Check(adapter_root == movement_root && clump == monster_clump,
              "clump render callback arguments");
        events.emplace_back("render callback");
    }
    void DispatchCombatPositions(std::uint32_t incoming,
                                 std::uint32_t bits) override {
        Check(incoming == monster, "combat-position monster");
        events.emplace_back("positions");
        dispatch_bits = bits;
    }
    void InitializeCombatSystem(std::uint32_t incoming,
                                std::uint32_t mode) override {
        Check(incoming == monster, "combat-system monster");
        events.emplace_back("combat");
        combat_mode = mode;
    }
    void UseBehavior(std::int32_t active_count) {
        words[monster + 1620u] = behavior;
        words[behavior + 64u] = static_cast<std::uint32_t>(active_count);
        words[behavior + 76u] = behavior_xml;
    }
    void Run() {
        Check(pal4::MonsterCombatParameterCallback::Process(
                  callback, 0u, *this) == 0u,
              "combat callback return");
    }
};

void CheckGates() {
    for (unsigned gate = 0u; gate < 6u; ++gate) {
        ProcessRuntime runtime;
        switch (gate) {
        case 0u: break;
        case 1u: runtime.words[ProcessRuntime::game + 396u] = 0u; break;
        case 2u: runtime.state_result = 0u; break;
        case 3u: runtime.words[ProcessRuntime::callback + 8u] = 0u; break;
        case 4u: runtime.words[ProcessRuntime::actor + 732u] = 1u; break;
        case 5u: runtime.words[ProcessRuntime::actor + 736u] = 1u; break;
        }
        const auto argument = gate == 0u ? 1u : 0u;
        Check(pal4::MonsterCombatParameterCallback::Process(
                  ProcessRuntime::callback, argument, runtime) == 0u &&
              runtime.events.empty() &&
              runtime.words.find(ProcessRuntime::movement_root + 88u) ==
                  runtime.words.end(),
              "combat gate stops before side effects");
    }
}

void CheckWorldModes() {
    ProcessRuntime no_behavior;
    no_behavior.Run();
    Check(no_behavior.events == (std::vector<std::string>{
              "flag", "clear", "create document", "world", "destroy document"}),
          "missing behavior preserves scratch cleanup without entering combat");
    for (const auto& case_data : {
             std::pair<double, std::uint32_t>{15.0, 1u},
             {90.0, 0u}, {180.0, 0u}}) {
        ProcessRuntime runtime;
        runtime.UseBehavior(1);
        runtime.angle = case_data.first / 57.29578;
        runtime.Run();
        Check(runtime.world_mode == case_data.second &&
              runtime.combat_mode ==
                  (case_data.second == 1u ? 2u : 0u) &&
              runtime.world_count == 1u &&
              runtime.world_path == "world/audio" &&
              runtime.words.at(ProcessRuntime::movement_root + 88u) == 1u &&
              runtime.dispatch_bits == 1008981770u &&
              runtime.passed_dot == 0.0f &&
              runtime.events == (std::vector<std::string>{
                  "flag", "clear", "create document", "world",
                  "attribute", "positions", "combat", "destroy document"}),
              "combat world angle and call order");
    }
    ProcessRuntime behind;
    behind.UseBehavior(1);
    behind.floats[ProcessRuntime::monster + 176u] = -1.0f;
    behind.world_path.clear();
    behind.Run();
    Check(behind.world_mode == 2u && behind.combat_mode == 1u &&
          behind.world_path == "x",
          "negative facing uses alternate world and empty-path fallback");

    struct ThresholdCase {
        double degrees;
        std::uint32_t world_mode;
    };
    for (const auto threshold : {
             ThresholdCase{44.9, 1u}, ThresholdCase{45.1, 0u},
             ThresholdCase{314.9, 0u}, ThresholdCase{315.1, 1u}}) {
        ProcessRuntime runtime;
        runtime.UseBehavior(1);
        runtime.floats[ProcessRuntime::monster_transform + 32u] = -1.0f;
        runtime.angle = threshold.degrees / 57.29578;
        runtime.Run();
        Check(runtime.world_mode == threshold.world_mode &&
              runtime.combat_mode ==
                  (threshold.world_mode == 1u ? 2u : 0u),
              "45/315-degree world-mode thresholds");
    }
}

void CheckScripts() {
    ProcessRuntime cached;
    cached.UseBehavior(1);
    cached.script_name = "BeginFight";
    cached.cache_hit = true;
    cached.csb_mode = true;
    cached.Run();
    Check(cached.received_cache_key == "scripts/[BeginFight]" &&
          cached.events == (std::vector<std::string>{
              "flag", "clear", "create document", "world",
              "attribute", "script init", "cache", "script cleanup",
              "positions", "combat", "destroy document"}),
          "cached script skips interpreter and continues combat");

    ProcessRuntime reported;
    reported.UseBehavior(1);
    reported.script_name = "BeginFight";
    reported.Run();
    Check(reported.received_script_path == "scripts/" &&
          reported.received_script_name == "BeginFight" &&
          reported.received_context == 0x7777u &&
          reported.reported_message == "scripts/[BeginFight]" &&
          reported.reported_function == "combat_start" &&
          reported.events == (std::vector<std::string>{
              "flag", "clear", "create document", "world",
              "attribute", "script init", "cache", "interpreter",
              "handle", "report", "release", "script cleanup",
              "positions", "combat", "destroy document"}),
          "uncached script loads, pushes and releases its module");

    ProcessRuntime missing;
    missing.UseBehavior(0);
    missing.script_name = "BeginFight";
    missing.interpreter = 0u;
    missing.Run();
    Check(missing.events == (std::vector<std::string>{
              "flag", "clear", "create document", "world",
              "attribute", "script init", "cache", "interpreter",
              "missing script", "script cleanup", "active character",
              "render callback", "destroy document"}),
          "missing script and inactive behavior return early");

    ProcessRuntime rejected;
    rejected.UseBehavior(2);
    rejected.script_name = "BeginFight";
    rejected.handle_result = false;
    rejected.Run();
    Check(rejected.events == (std::vector<std::string>{
              "flag", "clear", "create document", "world",
              "attribute", "script init", "cache", "interpreter",
              "handle", "missing script", "script cleanup",
              "positions", "combat", "destroy document"}),
          "interpreter validation failure follows missing-script path");

    ProcessRuntime empty;
    empty.UseBehavior(-1);
    empty.Run();
    Check(empty.events == (std::vector<std::string>{
              "flag", "clear", "create document", "world",
              "attribute", "positions", "combat", "destroy document"}),
          "empty script skips special active-character branch");

    ProcessRuntime throwing;
    throwing.UseBehavior(1);
    throwing.script_name = "BeginFight";
    throwing.throw_interpreter = true;
    bool caught = false;
    try {
        throwing.Run();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && !throwing.events.empty() &&
          throwing.events.back() == "destroy document",
          "exception unwinding destroys combat document");
}

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::MonsterCombatParameterCallback::Construct(
                  0x1000u, runtime) == 0x1000u &&
              runtime.words.at(0x1000u) == 0x849384u &&
              runtime.words.at(0x1004u) == 0u &&
              runtime.words.at(0x1008u) == 0u &&
              runtime.writes ==
                  (std::vector<std::uint32_t>{0x1004u, 0x1000u, 0x1008u}),
              "combat parameter callback layout and write order");
        CheckGates();
        CheckWorldModes();
        CheckScripts();
        std::cout << "monster_combat_parameter_callback_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
