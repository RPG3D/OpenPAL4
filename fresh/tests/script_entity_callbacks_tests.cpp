#include "pal4/script_entity_callbacks.h"

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

struct PlayerRuntime final : pal4::IScriptPlayerLockRuntime {
    std::uint32_t player_manager = 0x1000u;
    std::uint32_t current_index = 2u;
    std::uint32_t current_object = 0x5000u;
    unsigned actor_gets = 0;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return player_manager;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == player_manager + 0x2c50u) return current_index;
        if (address == 0x2005u) return current_object;
        throw std::runtime_error("unexpected player read");
    }
    void SetActionCode(std::uint32_t component,
                       std::string_view code) override {
        Check(component == player_manager + current_index * 2836u +
                               0x170u && code == "C01",
              "current player action component and code");
        calls.emplace_back("action:C01");
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor-control");
        return 0x2000u + actor_gets++;
    }
    void ResetAndSync(std::uint32_t control,
                      std::uint32_t flag) override {
        Check(control == 0x2000u && flag <= 1u,
              "actor reset operands");
        calls.push_back("reset:" + std::to_string(flag));
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui-manager");
        return 0x3000u;
    }
    std::uint8_t ProcessUiEvents(std::uint32_t manager,
                                  std::uint8_t flag) override {
        Check(manager == 0x3000u && flag <= 1u, "ui event operands");
        calls.push_back("process-ui:" + std::to_string(flag));
        return 0xa5u;
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap-manager");
        return 0x4000u;
    }
    void UpdateGrid(std::uint32_t manager, std::uint32_t position,
                    std::uint32_t direction) override {
        Check(manager == 0x4000u && position == current_object + 0xacu &&
              direction == current_object + 0xa0u,
              "grid position and direction order");
        calls.emplace_back("update-grid");
    }
};

struct NpcRuntime final : pal4::IScriptNpcVisibilityRuntime {
    std::uint32_t npc = 0x7000u;
    unsigned arena_gets = 0;
    std::vector<std::string> calls;
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc-manager");
        return 0x6000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x6000u, "npc lookup manager");
        calls.push_back("find:" + std::string(name));
        return npc;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x8000u + arena_gets++;
    }
    std::uint32_t LeaveNpcScript(std::uint32_t arena,
                                 std::uint32_t value) override {
        Check(arena == 0x8000u && value == npc, "npc remove operands");
        calls.emplace_back("leave-script");
        return 0x41u;
    }
    void SetStateFlag(std::uint32_t value,
                      std::uint32_t flag) override {
        Check(value == npc && flag == 1u, "npc visible state flag");
        calls.emplace_back("state-flag");
    }
    std::uint32_t EnterNpcScript(std::uint32_t arena,
                                 std::uint32_t value) override {
        Check(arena == 0x8001u && value == npc,
              "npc add uses fresh arena manager");
        calls.emplace_back("enter-script");
        return 0x42u;
    }
};

void PlayerLock() {
    PlayerRuntime runtime;
    Check(pal4::ScriptPlayerLockHandler::Lock(runtime) == 0xa5u &&
          runtime.calls == std::vector<std::string>{
              "player-manager", "read:15440", "action:C01",
              "actor-control", "reset:0", "ui-manager", "process-ui:0"},
          "lock sets action before processing actor and UI with zero flag");

    PlayerRuntime absent;
    absent.player_manager = 0;
    absent.current_index = 0;
    (void)pal4::ScriptPlayerLockHandler::Lock(absent);
    Check(absent.calls == std::vector<std::string>{
              "player-manager", "read:11344", "actor-control", "reset:0",
              "ui-manager", "process-ui:0"},
          "zero computed player skips action but keeps lock processing");
}

void PlayerUnlock() {
    PlayerRuntime runtime;
    pal4::ScriptPlayerLockHandler::Unlock(runtime);
    Check(runtime.calls == std::vector<std::string>{
              "actor-control", "reset:1", "ui-manager", "process-ui:1",
              "actor-control", "read:8197", "minimap-manager", "update-grid"},
          "unlock processing precedes current object grid refresh");

    PlayerRuntime absent;
    absent.current_object = 0;
    pal4::ScriptPlayerLockHandler::Unlock(absent);
    Check(absent.calls == std::vector<std::string>{
              "actor-control", "reset:1", "ui-manager", "process-ui:1",
              "actor-control", "read:8197"},
          "missing current object skips minimap");
}

void NpcVisibility() {
    NpcRuntime missing;
    missing.npc = 0;
    Check(pal4::ScriptNpcVisibility::SetVisible(
              "npc", true, missing) == 0u &&
          missing.calls == std::vector<std::string>{
              "npc-manager", "find:npc"},
          "missing NPC does not touch arena");

    NpcRuntime hidden;
    Check(pal4::ScriptNpcVisibility::SetVisible(
              "npc", false, hidden) == 0x41u &&
          hidden.calls == std::vector<std::string>{
              "npc-manager", "find:npc", "arena-manager", "leave-script"},
          "hide only removes NPC script");

    NpcRuntime visible;
    Check(pal4::ScriptNpcVisibility::SetVisible(
              nullptr, true, visible) == 0x42u &&
          visible.calls == std::vector<std::string>{
              "npc-manager", "find:", "arena-manager", "leave-script",
              "state-flag", "arena-manager", "enter-script"},
          "null name falls back to empty and show removes before adding");
}

}  // namespace

int main() {
    try {
        PlayerLock();
        PlayerUnlock();
        NpcVisibility();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
