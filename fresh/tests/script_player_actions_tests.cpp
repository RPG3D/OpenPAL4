#include "pal4/script_player_actions.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerActionsRuntime {
    std::uint32_t player = 0x8000u;
    std::uint32_t property = 0x9000u;
    std::uint32_t attached = 1u;
    unsigned arena_calls = 0;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return 0x1000u;
    }
    std::uint32_t UpdatePlayerStates(std::uint32_t manager,
                                     std::int32_t index) override {
        Check(manager == 0x1000u && index == -7, "leader operands");
        calls.emplace_back("update-player-states");
        return 0x31u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == -7, "lookup operands");
        calls.emplace_back("player-by-index");
        return player;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x3000u + arena_calls++;
    }
    std::uint32_t ScriptOnRemove(std::uint32_t arena,
                                 std::uint32_t value) override {
        Check(arena == 0x3000u && value == player,
              "script remove operands");
        calls.emplace_back("script-remove");
        return 0x41u;
    }
    std::uint32_t ScriptOnAdd(std::uint32_t arena,
                              std::uint32_t value) override {
        Check(arena == 0x3001u && value == player,
              "script add uses second arena manager");
        calls.emplace_back("script-add");
        return 0x42u;
    }
    void SetStateFlag(std::uint32_t value,
                      std::uint32_t flag) override {
        Check(value == player && flag == 1u, "visible state flag");
        calls.emplace_back("state-flag");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == player + 496u) return property;
        if (address == player + 2832u) return attached;
        if (address == player + 20u) return 0xa000u;
        if (address == 0x4000u) return 0x5000u;
        throw std::runtime_error("unexpected player read");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == player + 2832u, "attachment offset");
        calls.push_back("write-attachment:" + std::to_string(value));
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("movement-adapter");
        return 0x4000u;
    }
    std::uint32_t SetClumpProperties(
        std::uint32_t adapter, std::uint32_t clump,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third, std::uint32_t fourth) override {
        Check(adapter == 0x5000u && clump == 0xa000u &&
              first == 1u && second == 1u && third == 0u && fourth == 0u,
              "clump collision properties");
        calls.emplace_back("set-clump-properties");
        return 0x43u;
    }
    std::uint32_t SetClumpRenderCallback(std::uint32_t adapter,
                                         std::uint32_t clump) override {
        Check(adapter == 0x5000u && clump == 0xa000u,
              "clump render callback operands");
        calls.emplace_back("set-clump-callback");
        return 0x44u;
    }
};

void SetLeader() {
    Runtime runtime;
    Check(pal4::ScriptPlayerActions::SetLeader(-7, runtime) == 0x31u &&
          runtime.calls == std::vector<std::string>{
              "player-manager", "update-player-states"},
          "leader update forwards signed index and result");
}

void Visibility() {
    Runtime missing;
    missing.player = 0;
    Check(pal4::ScriptPlayerActions::SetVisible(-7, true, missing) == 0u &&
          missing.calls == std::vector<std::string>{
              "player-manager", "player-by-index"},
          "missing player has no arena side effects");

    Runtime hidden;
    Check(pal4::ScriptPlayerActions::SetVisible(-7, false, hidden) == 0x41u &&
          hidden.calls == std::vector<std::string>{
              "player-manager", "player-by-index", "arena-manager",
              "script-remove"},
          "hide only removes player script");

    Runtime visible;
    Check(pal4::ScriptPlayerActions::SetVisible(-7, true, visible) == 0x42u &&
          visible.calls == std::vector<std::string>{
              "player-manager", "player-by-index", "arena-manager",
              "script-remove", "state-flag", "arena-manager", "script-add"},
          "show removes, marks, then adds using fresh arena manager");
}

void Collision() {
    Runtime missing;
    missing.player = 0;
    Check(pal4::ScriptPlayerActions::AttachCollision(-7, missing) == 0u &&
          missing.calls.size() == 2u,
          "missing player skips collision attachment");

    Runtime no_property;
    no_property.property = 0;
    Check(pal4::ScriptPlayerActions::AttachCollision(-7, no_property) == 0u &&
          no_property.calls == std::vector<std::string>{
              "player-manager", "player-by-index", "read:33264"},
          "missing player property skips movement adapter");

    Runtime attached;
    Check(pal4::ScriptPlayerActions::AttachCollision(-7, attached) == 0x43u &&
          attached.calls == std::vector<std::string>{
              "player-manager", "player-by-index", "read:33264",
              "read:32788", "movement-adapter", "read:16384",
              "set-clump-properties", "write-attachment:1"},
          "attach reads clump, sets collision and marks state");

    Runtime detached_state;
    detached_state.attached = 0;
    Check(pal4::ScriptPlayerActions::DetachCollision(-7, detached_state) == 0u &&
          detached_state.calls == std::vector<std::string>{
              "player-manager", "player-by-index", "read:35600"},
          "clear attachment state skips adapter");

    Runtime detached;
    Check(pal4::ScriptPlayerActions::DetachCollision(-7, detached) == 0x44u &&
          detached.calls == std::vector<std::string>{
              "player-manager", "player-by-index", "read:35600",
              "read:32788", "movement-adapter", "read:16384",
              "set-clump-callback", "write-attachment:0"},
          "detach sets callback before clearing state");
}

}  // namespace

int main() {
    try {
        SetLeader();
        Visibility();
        Collision();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
