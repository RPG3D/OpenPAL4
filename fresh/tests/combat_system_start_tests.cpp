#include "pal4/combat_system_start.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kConfig = 0x2000u;
constexpr std::uint32_t kManager = 0x3000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemStartRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::pair<std::uint32_t, std::string>, std::int32_t> integers;
    std::vector<std::uint32_t> children;
    std::array<std::uint32_t, 4> players{};
    std::vector<std::string> calls;
    bool has_scene = true;
    bool has_area = true;

    Runtime() {
        words[kConfig + 56u] = 1u;
        words[kObject + 396u] = 0x7000u;
        words[kObject + 412u] = 0x7100u;
        words[kObject + 416u] = 0x7200u;
        words[kObject + 424u] = 0x7300u;
        words[kManager + 52u] = 0x8000u;
        words[kManager + 56u] = 0x8004u;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void GetSkillSystem() override { calls.emplace_back("skill"); }
    std::uint32_t GetActorManager() override {
        calls.emplace_back("manager");
        return kManager;
    }
    void ResetTaskFlags(std::uint32_t queue) override {
        Check(queue == kManager + 96u, "reset queue offset");
        calls.emplace_back("reset-flags");
    }
    void CleanupTaskQueue(std::uint32_t queue) override {
        Check(queue == kManager + 96u, "cleanup queue offset");
        calls.emplace_back("cleanup-queue");
    }
    std::optional<std::string> FindAttributeString(
        std::uint32_t node, std::string_view key) override {
        Check(node == kConfig, "required string lookup config");
        calls.emplace_back("find:" + std::string(key));
        if (key == "CombatScene")
            return has_scene ? std::optional<std::string>("scene_a") :
                               std::nullopt;
        if (key == "CombatArea")
            return has_area ? std::optional<std::string>("area_b") :
                              std::nullopt;
        throw std::runtime_error("unexpected required string key");
    }
    std::int32_t GetConfigInt(std::uint32_t node,
                              std::string_view key,
                              std::int32_t fallback) override {
        calls.emplace_back("int:" + std::string(key) + ":" +
                           std::to_string(node));
        const auto it = integers.find({node, std::string(key)});
        return it == integers.end() ? fallback : it->second;
    }
    void InitializeCold(std::uint32_t object) override {
        Check(object == kObject, "cold init combat object");
        calls.emplace_back("cold-init");
        bytes[object + 517u] = 1u;
    }
    void ProcessCollections(std::uint32_t collection) override {
        Check(collection == 0x7100u, "collection pointer +412");
        calls.emplace_back("collections");
    }
    void InitializeGameStats(std::uint32_t handler) override {
        Check(handler == 0x7200u, "stat handler pointer +416");
        calls.emplace_back("stats");
    }
    void UpdateSystemManager(std::uint32_t manager) override {
        Check(manager == 0x7000u, "system manager pointer +396");
        calls.emplace_back("system-state");
    }
    void ClearCombatRosters(std::uint32_t manager) override {
        Check(manager == kManager, "actor reset target");
        calls.emplace_back("actor-reset");
    }
    std::uint32_t GetConsole() override {
        calls.emplace_back("console");
        return 0x7400u;
    }
    void ResetConsole(std::uint32_t console) override {
        Check(console == 0x7400u, "console reset target");
        calls.emplace_back("console-reset");
    }
    void SetupCameraDefaults() override { calls.emplace_back("camera"); }
    void UnsubscribeConsoleEvents(std::uint32_t console) override {
        Check(console == 0x7400u, "console unsubscribe target");
        calls.emplace_back("unsubscribe");
    }
    void SubscribeConsoleEvents(std::uint32_t console) override {
        Check(console == 0x7400u, "console subscribe target");
        calls.emplace_back("subscribe");
    }
    void DispatchScriptEvent(std::string_view name,
                             std::int32_t value) override {
        calls.emplace_back("event:" + std::string(name) + ":" +
                           std::to_string(value));
    }
    std::uint32_t GetCombatSystemInstance() override {
        calls.emplace_back("instance");
        return kObject;
    }
    void LoadWorld(std::uint32_t object, std::string_view scene,
                   std::string_view area) override {
        Check(object == kObject && scene == "scene_a" && area == "area_b",
              "world uses both required strings in order");
        calls.emplace_back("load-world");
    }
    void InitializeGameManager(std::uint32_t manager) override {
        Check(manager == 0x7300u, "game manager pointer +424");
        calls.emplace_back("game-manager");
    }
    void ShowCombatUi(std::uint32_t object) override {
        Check(object == kObject, "show combat UI object");
        calls.emplace_back("show-ui");
    }
    std::uint32_t GetChildByIndex(std::uint32_t config,
                                  std::uint32_t index) override {
        Check(config == kConfig && index < children.size(),
              "child lookup index");
        calls.emplace_back("child:" + std::to_string(index));
        return children[index];
    }
    void CreateMonster(std::uint32_t manager, std::int32_t monster_id,
                       std::uint32_t position,
                       std::int32_t ab_state) override {
        Check(manager == kManager, "monster collection manager");
        calls.emplace_back("monster:" + std::to_string(monster_id) + ":" +
                           std::to_string(position) + ":" +
                           std::to_string(ab_state));
    }
    std::uint32_t GetPlayer(std::uint32_t index) override {
        Check(index < 4u, "player index 0..3");
        calls.emplace_back("player:" + std::to_string(index));
        return players[index];
    }
    void SortPlayers(std::vector<std::uint32_t>& selected) override {
        calls.emplace_back("sort");
        std::sort(selected.begin(), selected.end(), std::greater<>{});
    }
    void PreparePlayer(std::uint32_t player) override {
        calls.emplace_back("prepare-player:" + std::to_string(player));
    }
    void CreatePlayerActor(std::uint32_t collection,
                           std::uint32_t player,
                           std::uint32_t position) override {
        Check(collection == kManager + 32u,
              "player actor collection offset");
        calls.emplace_back("actor:" + std::to_string(player) + ":" +
                           std::to_string(position));
    }
    std::uint32_t GetRenderCallback() override {
        calls.emplace_back("get-render");
        return 0x76543210u;
    }
    void SetRenderCallback(std::uint32_t callback) override {
        Check(callback == 0x574df0u, "combat render callback address");
        calls.emplace_back("set-render");
    }
    void ResetAudioPosition() override { calls.emplace_back("audio"); }
};

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& target) {
    const auto it = std::find(calls.begin(), calls.end(), target);
    Check(it != calls.end(), "required call missing");
    return static_cast<std::size_t>(it - calls.begin());
}

void EarlyGates() {
    Runtime absent;
    absent.bytes[kObject + 501u] = 0xabu;
    Check(!pal4::CombatSystemStart::Start(
              kObject, 0u, 0x123u, absent),
          "null config fails");
    Check(absent.bytes[kObject + 516u] == 0x23u &&
              absent.bytes[kObject + 519u] == 0xabu &&
              absent.words[kObject + 508u] == 7u &&
              absent.words[kObject + 504u] == 1u &&
              absent.words[kObject + 388u] == 0u &&
              absent.words[kObject + 392u] == 0u &&
              absent.bytes[kObject + 501u] == 0u &&
              absent.calls.size() == 14u,
          "early failure retains all pre-gate writes");

    Runtime no_children;
    no_children.words[kConfig + 56u] = 0u;
    no_children.bytes[kObject + 502u] = 1u;
    no_children.bytes[kObject + 501u] = 0x66u;
    Check(!pal4::CombatSystemStart::Start(
              kObject, kConfig, 0u, no_children) &&
              no_children.bytes[kObject + 501u] == 0x66u,
          "zero child count fails and preserve flag keeps byte +501");

    Runtime no_scene;
    no_scene.has_scene = false;
    Check(!pal4::CombatSystemStart::Start(
              kObject, kConfig, 0u, no_scene) &&
              IndexOf(no_scene.calls, "find:CombatScene") ==
                  no_scene.calls.size() - 1u,
          "missing CombatScene fails before CombatArea");
    Runtime no_area;
    no_area.has_area = false;
    Check(!pal4::CombatSystemStart::Start(
              kObject, kConfig, 0u, no_area) &&
              IndexOf(no_area.calls, "find:CombatArea") ==
                  no_area.calls.size() - 1u,
          "missing CombatArea fails before cold init");
}

void FullStart() {
    Runtime runtime;
    runtime.children = {0x4000u, 0x4100u, 0x4200u,
                        0x4300u, 0x4400u, 0u};
    runtime.words[kConfig + 56u] =
        static_cast<std::uint32_t>(runtime.children.size());
    runtime.integers[{kConfig, "StartType"}] = 3;
    runtime.integers[{0x4000u, "MonsterId"}] = 11;
    runtime.integers[{0x4000u, "CombatPosition"}] = 0;
    runtime.integers[{0x4000u, "MonsterAbState"}] = 2;
    runtime.integers[{0x4100u, "MonsterId"}] = 22;
    runtime.integers[{0x4100u, "CombatPosition"}] = 0;
    runtime.integers[{0x4100u, "MonsterAbState"}] = 3;
    runtime.integers[{0x4200u, "MonsterId"}] = 33;
    runtime.integers[{0x4200u, "CombatPosition"}] = 5;
    runtime.integers[{0x4200u, "MonsterAbState"}] = 4;
    runtime.integers[{0x4300u, "MonsterId"}] = 44;
    runtime.integers[{0x4300u, "CombatPosition"}] = 9;
    runtime.integers[{0x4300u, "MonsterAbState"}] = 5;
    runtime.integers[{0x4400u, "MonsterId"}] = 0;
    runtime.players = {0x5000u, 0x5100u, 0u, 0x5200u};
    runtime.words[0x5000u + 2824u] = 1u;
    runtime.words[0x5100u + 2824u] = 1u;
    runtime.words[0x5200u + 2824u] = 0u;

    Check(pal4::CombatSystemStart::Start(kObject, kConfig, 0u, runtime),
          "populated combat commits");
    Check(runtime.calls[0] == "skill" &&
              runtime.calls[1] == "manager" &&
              runtime.calls[2] == "reset-flags" &&
              runtime.calls[3] == "manager" &&
              runtime.calls[4] == "cleanup-queue" &&
              IndexOf(runtime.calls, "cold-init") <
                  IndexOf(runtime.calls, "collections") &&
              IndexOf(runtime.calls, "event:combat_reset:0") <
                  IndexOf(runtime.calls, "load-world") &&
              IndexOf(runtime.calls, "load-world") <
                  IndexOf(runtime.calls, "show-ui"),
          "reset, initialization, event and UI order");
    const auto& calls = runtime.calls;
    Check(IndexOf(calls, "monster:11:0:2") <
                  IndexOf(calls, "monster:33:5:4") &&
              IndexOf(calls, "monster:33:5:4") <
                  IndexOf(calls, "monster:22:6:3") &&
              IndexOf(calls, "monster:22:6:3") <
                  IndexOf(calls, "monster:44:6:5") &&
              std::count_if(calls.begin(), calls.end(),
                            [](const auto& call) {
                                return call.rfind("monster:", 0u) == 0u;
                            }) == 4,
          "occupied and out-of-range monsters are deferred in child order");
    Check(IndexOf(calls, "prepare-player:20736") <
                  IndexOf(calls, "actor:20736:6") &&
              IndexOf(calls, "actor:20736:6") <
                  IndexOf(calls, "prepare-player:20480") &&
              IndexOf(calls, "prepare-player:20480") <
                  IndexOf(calls, "actor:20480:6"),
          "active players are sorted and pre-stepped before actor creation");
    Check(runtime.words[kObject + 428u] == 0x76543210u &&
              runtime.words[kObject + 508u] == 1u &&
              IndexOf(calls, "set-render") <
                  IndexOf(calls, "event:combat_prepare:3") &&
              IndexOf(calls, "event:combat_prepare:3") <
                  IndexOf(calls, "audio"),
          "ready commit saves and installs callback before prepare event");
}

void ActorCountFailureAndInactive() {
    Runtime runtime;
    runtime.children = {0x4000u};
    runtime.bytes[kObject + 517u] = 1u;
    runtime.words[kManager + 56u] = runtime.words[kManager + 52u];
    runtime.players[0] = 0x5000u;
    runtime.words[0x5000u + 2824u] = 1u;
    Check(!pal4::CombatSystemStart::Start(
              kObject, kConfig, 1u, runtime),
          "empty actor collection fails ready gate");
    Check(IndexOf(runtime.calls, "actor:20480:1") <
                  runtime.calls.size() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "cold-init") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "load-world") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "subscribe") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "set-render") == runtime.calls.end() &&
              runtime.words[kObject + 508u] == 7u,
          "inactive mode skips world and subscriptions; failed gate skips commit");

    runtime.calls.clear();
    runtime.words[kManager + 52u] = 0u;
    runtime.words[kManager + 56u] = 4u;
    Check(!pal4::CombatSystemStart::Start(
              kObject, kConfig, 1u, runtime),
          "null actor vector fails even when end differs");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        FullStart();
        ActorCountFailureAndInactive();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
