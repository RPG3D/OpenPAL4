#include "pal4/world_state.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t Position(const std::vector<std::string>& calls,
                     const std::string& target) {
    const auto it = std::find(calls.begin(), calls.end(), target);
    return static_cast<std::size_t>(it - calls.begin());
}

struct FrameRuntime final : pal4::IWorldStateFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t frame_value = 0x1234u;
    FrameRuntime() { words[0x1000u + 196u] = 1u; words[0x5000u] = frame_value; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t ArenaManager() override { calls.emplace_back("arena"); return 0x1000u; }
    void ProcessWorldFrame(std::uint32_t arena, std::uint32_t value) override {
        Check(arena == 0x1000u && value == frame_value, "world frame arguments");
        calls.emplace_back("world-frame");
    }
    std::uint32_t Minimap() override { calls.emplace_back("minimap"); return 2u; }
    void DrawMinimap(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 2u && frame == 0x5000u, "minimap arguments");
        calls.emplace_back("draw-minimap");
    }
    std::uint32_t ScreenEffect() override { calls.emplace_back("effect"); return 3u; }
    void DrawScreenEffect(std::uint32_t effect, std::uint32_t frame) override {
        Check(effect == 3u && frame == 0x5000u, "effect render arguments");
        calls.emplace_back("draw-effect");
    }
    void AdvanceScreenEffect(std::uint32_t effect, float delta) override {
        Check(effect == 3u && delta == 0.25f, "effect update arguments");
        calls.emplace_back("advance-effect");
    }
    std::uint32_t BinkPlayer() override { calls.emplace_back("bink"); return 4u; }
    void RenderBink(std::uint32_t player, std::uint32_t frame) override {
        Check(player == 4u && frame == 0x5000u, "Bink arguments");
        calls.emplace_back("render-bink");
    }
    std::uint32_t UIFrameManager() override { calls.emplace_back("ui"); return 5u; }
    void RenderUI(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 5u && frame == 0x5000u, "UI render arguments");
        calls.emplace_back("render-ui");
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 5u && delta == 0.25, "UI update arguments");
        calls.emplace_back("update-ui");
    }
    void RenderScene(std::uint32_t frame) override {
        Check(frame == 0x5000u, "scene frame argument");
        calls.emplace_back("render-scene");
    }
    std::uint32_t Console() override { calls.emplace_back("console"); return 6u; }
    void RenderConsole(std::uint32_t console, std::uint32_t frame) override {
        Check(console == 6u && frame == 0x5000u, "console arguments");
        calls.emplace_back("render-console");
    }
    std::uint32_t ScriptExecutor() override { calls.emplace_back("script"); return 0x2000u; }
    void ProcessLog(std::uint32_t logger, double delta) override {
        Check(logger == 0x2054u && delta == 0.25, "log arguments");
        calls.emplace_back("log");
    }
    void ProcessTimedScripts(std::uint32_t executor, float delta) override {
        Check(executor == 0x2000u && delta == 0.25f, "timed script arguments");
        calls.emplace_back("timed-scripts");
    }
    std::uint32_t CameraManager() override { calls.emplace_back("camera-manager"); return 7u; }
    std::uint32_t ActiveCamera(std::uint32_t manager) override {
        Check(manager == 7u, "camera manager argument");
        calls.emplace_back("active-camera"); return 8u;
    }
    void UpdateCamera(std::uint32_t camera, float delta) override {
        Check(camera == 8u && delta == 0.25f, "camera update arguments");
        calls.emplace_back("update-camera");
    }
    void UpdateWorld(std::uint32_t arena, float delta,
                     std::uint32_t mode, std::uint32_t flag) override {
        Check(arena == 0x1000u && delta == 0.25f && mode == 9u && flag <= 1u,
              "world update arguments");
        calls.emplace_back("update-world");
    }
};

struct LifecycleRuntime final : pal4::IWorldStateLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t monster = 0x9000u;
    std::uint32_t character_result = 0x7777u;
    LifecycleRuntime() {
        words[0x2000u + 224u] = 0xaaaa;
        words[0x2000u + 8u] = 0xbbbb;
        words[0x3000u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override { return words[address]; }
    std::uint8_t ReadByte(std::uint32_t address) override { return bytes[address]; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t UIFrameManager() override { return 0x4000u; }
    void SetUIFlag(std::uint32_t manager, std::uint32_t flag) override {
        Check(manager == 0x4000u && flag == 0u, "entry UI flag");
        calls.emplace_back("ui-flag");
    }
    void SendUIEvent(std::uint32_t manager, std::uint32_t event,
                     std::uint32_t first, std::uint32_t second) override {
        Check(manager == 0x4000u && second == 0u, "UI event arguments");
        calls.emplace_back("event:" + std::to_string(event) + ":" + std::to_string(first));
    }
    std::uint32_t CombatSystem() override { return 0x5000u; }
    std::uint32_t MonsterManager() override { return 0x6000u; }
    void SetMonsterFlags(std::uint32_t manager) override {
        Check(manager == 0x6000u, "set monster flags manager");
        calls.emplace_back("set-monster-flags");
    }
    void ClearMonsterFlags(std::uint32_t manager) override {
        Check(manager == 0x6000u, "clear monster flags manager");
        calls.emplace_back("clear-monster-flags");
    }
    std::uint8_t CombatProperty519(std::uint32_t system) override {
        Check(system == 0x5000u, "combat property system");
        calls.emplace_back("combat-property"); return bytes[0x5000u + 519u];
    }
    std::uint32_t ActorControl() override { return 0x3000u; }
    void EnableInvincibility(std::uint32_t control, float seconds) override {
        Check(control == 0x3000u && seconds == 5.0f, "invincibility arguments");
        calls.emplace_back("invincibility");
    }
    std::uint32_t GameInstance() override { return 0x7000u; }
    void ForEachStateMonster(std::uint32_t game) override {
        Check(game == 0x7000u, "state monster collection");
        calls.emplace_back("foreach-monster");
    }
    std::uint32_t ArenaManager() override { return 0x2000u; }
    std::uint32_t BackgroundMusicPlayer() override {
        calls.emplace_back("music-player"); return 0x8000u;
    }
    void SetBackgroundMusic(std::uint32_t player, std::uint32_t arena_player,
                            std::uint32_t scene_name) override {
        Check(player == 0x8000u && arena_player == 0xbbbbu &&
              scene_name == 0xaaaau, "background music arguments");
        calls.emplace_back("music");
    }
    void ClearStateCollection(std::uint32_t game) override {
        Check(game == 0x7000u, "state collection argument");
        calls.emplace_back("clear-states");
    }
    std::uint32_t CameraManager() override { return 0x8100u; }
    void ActivateCamera(std::uint32_t manager, std::uint32_t name) override {
        Check(manager == 0x8100u && name == 0x8200u, "camera activation arguments");
        calls.emplace_back("activate-camera");
    }
    std::uint32_t DefaultCameraName() override { return 0x8200u; }
    std::uint32_t ActorMessageManager() override { return 0x8300u; }
    void ClearActorMessages(std::uint32_t manager) override {
        Check(manager == 0x8300u, "message manager argument");
        calls.emplace_back("clear-messages");
    }
    void UpdateActorComponents(std::uint32_t control) override {
        Check(control == 0x3000u, "actor update argument");
        calls.emplace_back("update-components");
    }
    void ProcessActorInput(std::uint32_t control) override {
        Check(control == 0x3000u, "actor input argument");
        calls.emplace_back("process-input");
    }
    void ClearActorInputVector(std::uint32_t control) override {
        Check(control == 0x3000u, "actor clear argument");
        calls.emplace_back("clear-input");
    }
    std::uint32_t InputManager() override { return 0x8400u; }
    void InitializeInput(std::uint32_t manager) override {
        Check(manager == 0x8400u, "input manager argument");
        calls.emplace_back("initialize-input");
    }
    std::uint32_t Milliseconds() override { calls.emplace_back("time"); return 12345u; }
    std::uint32_t GameState() override { return 0x8500u; }
    std::uint32_t NpcManager() override { return 0x8600u; }
    void SetNpcGridUpdating(std::uint32_t grid, bool enabled) override {
        Check(grid == 0x8600u + 56u, "NPC grid address");
        calls.emplace_back(enabled ? "grid-on" : "grid-off");
    }
    void ProcessMonsterCollisions(std::uint32_t manager, float delta) override {
        Check(manager == 0x6000u && delta == 0.1f, "collision arguments");
        calls.emplace_back("collisions");
    }
    std::uint32_t GameObjectManager() override { return 0x8700u; }
    void SetGameObjectsActive(std::uint32_t manager, bool active) override {
        Check(manager == 0x8700u, "game object manager argument");
        calls.emplace_back(active ? "objects-on" : "objects-off");
    }
    std::uint32_t TemperatureManager() override { return 0x8800u; }
    std::uint32_t FindMonster(std::uint32_t manager, const char* name) override {
        Check(manager == 0x6000u && std::string(name) == "target", "monster lookup arguments");
        calls.emplace_back("find-monster"); return monster;
    }
    std::uint32_t LeaveCharacter(std::uint32_t arena, std::uint32_t found) override {
        Check(arena == 0x2000u && found == monster, "leave character arguments");
        calls.emplace_back("leave-character"); return character_result;
    }
};

void FrameTests() {
    FrameRuntime runtime;
    pal4::WorldState::Render(0x5000u, runtime);
    const std::vector<std::string> order = {"world-frame", "draw-minimap",
        "draw-effect", "render-bink", "render-ui", "render-scene", "render-console"};
    for (std::size_t i = 1; i < order.size(); ++i)
        Check(Position(runtime.calls, order[i - 1]) < Position(runtime.calls, order[i]),
              "render service order");
    runtime.calls.clear(); runtime.words[0x5000u] = 0u;
    pal4::WorldState::Render(0x5000u, runtime);
    Check(Position(runtime.calls, "world-frame") == runtime.calls.size(),
          "zero frame skips rendering");
    runtime.calls.clear(); runtime.words[0x5000u] = runtime.frame_value;
    runtime.words[0x1000u + 196u] = 0u;
    pal4::WorldState::Render(0x5000u, runtime);
    Check(Position(runtime.calls, "world-frame") == runtime.calls.size(),
          "inactive arena skips rendering");
    pal4::WorldState::Update(0.25f, 9u, 0u, runtime);
    Check(Position(runtime.calls, "log") < Position(runtime.calls, "timed-scripts") &&
          Position(runtime.calls, "timed-scripts") < Position(runtime.calls, "update-camera") &&
          Position(runtime.calls, "update-camera") < Position(runtime.calls, "advance-effect") &&
          Position(runtime.calls, "update-ui") == runtime.calls.size(),
          "inactive update order and gate");
    runtime.calls.clear(); runtime.words[0x1000u + 196u] = 1u;
    pal4::WorldState::Update(0.25f, 9u, 0u, runtime);
    Check(Position(runtime.calls, "advance-effect") < Position(runtime.calls, "update-world") &&
          Position(runtime.calls, "update-world") < Position(runtime.calls, "update-ui"),
          "active update order");
    runtime.calls.clear();
    pal4::WorldState::Update(0.25f, 9u, 1u, runtime);
    Check(Position(runtime.calls, "advance-effect") == runtime.calls.size() &&
          Position(runtime.calls, "update-world") < runtime.calls.size(),
          "flag suppresses effect only");
}

void LifecycleTests() {
    LifecycleRuntime runtime;
    Check(pal4::WorldState::Construct(0x100u, runtime) == 0x100u &&
          runtime.words[0x100u] == pal4::WorldState::kVtable,
          "world state constructor");
    runtime.calls.clear();
    Check(pal4::WorldState::EnterWrapper(0x100u, 99u, 0u, runtime) == 1u,
          "ordinary entry return");
    Check(Position(runtime.calls, "ui-flag") < Position(runtime.calls, "clear-states") &&
          Position(runtime.calls, "music") == runtime.calls.size() &&
          Position(runtime.calls, "grid-off") < Position(runtime.calls, "collisions") &&
          runtime.words[0x8500u + 1388u] == 12345u &&
          runtime.words[0x8500u + 1392u] == 12345u &&
          runtime.words[0x8500u + 1396u] == 12345u,
          "ordinary entry and timestamp writes");
    Check(Position(runtime.calls, "event:" + std::to_string(pal4::WorldState::kUiActorEvent) + ":1") <
          Position(runtime.calls, "clear-messages"), "actor event before message clear");
    for (std::uint32_t result = 0; result <= 3; ++result) {
        runtime.calls.clear(); runtime.words[0x5000u + 504u] = result;
        Check(pal4::WorldState::Enter(3u, runtime) == 1u, "combat return entry");
        Check(Position(runtime.calls, "music") < Position(runtime.calls, "clear-states"),
              "combat music before collection clear");
        Check((Position(runtime.calls, "set-monster-flags") < runtime.calls.size()) ==
                  (result == 0u || result == 2u), "combat result monster flags");
        Check((Position(runtime.calls, "foreach-monster") < runtime.calls.size()) ==
                  (result == 1u), "combat result retained monster removal");
        Check((Position(runtime.calls, "invincibility") < runtime.calls.size()) ==
                  (result == 2u), "combat result invincibility");
    }
    runtime.calls.clear(); runtime.bytes[0x5000u + 519u] = 1u;
    runtime.bytes[0x8800u + 40u] = 1u;
    runtime.words[0x5000u + 504u] = 2u;
    Check(pal4::WorldState::Enter(3u, runtime) == 1u,
          "gated combat return entry");
    Check(Position(runtime.calls, "invincibility") == runtime.calls.size() &&
          Position(runtime.calls, "event:" + std::to_string(pal4::WorldState::kUiTemperatureEvent) + ":1") < runtime.calls.size(),
          "combat property and temperature gates");
    runtime.calls.clear();
    Check(pal4::WorldState::LeaveWrapper(0x100u, 99u, runtime) == 1u,
          "world leave return");
    Check(Position(runtime.calls, "event:" + std::to_string(pal4::WorldState::kUiActorEvent) + ":0") <
          Position(runtime.calls, "clear-messages") &&
          Position(runtime.calls, "grid-on") < Position(runtime.calls, "process-input") &&
          Position(runtime.calls, "objects-off") < Position(runtime.calls, "time") &&
          Position(runtime.calls, "time") <
          Position(runtime.calls, "event:" + std::to_string(pal4::WorldState::kUiTemperatureEvent) + ":0"),
          "world leave service order");
    runtime.calls.clear(); runtime.monster = 0u;
    Check(pal4::WorldState::RemoveMonsterByName("target", runtime) == 0u &&
          Position(runtime.calls, "leave-character") == runtime.calls.size(),
          "missing monster return");
    runtime.calls.clear(); runtime.monster = 0x9000u;
    Check(pal4::WorldState::RemoveMonsterByName("target", runtime) == 0x7777u &&
          Position(runtime.calls, "find-monster") < Position(runtime.calls, "leave-character"),
          "found monster leave return");
}

}  // namespace

int main() {
    try {
        FrameTests();
        LifecycleTests();
        std::cout << "world state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
