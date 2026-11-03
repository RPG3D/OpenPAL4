#include "pal4/script_action_hold_release.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptActionHoldRuntime {
    std::uint32_t player = 0x2000u;
    std::uint32_t npc = 0x3000u;
    std::uint32_t name = 0x7000u;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player manager"); return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == 3,
              "player lookup arguments");
        calls.emplace_back("find player"); return player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5004u, "script string payload +4");
        calls.emplace_back("name"); return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty name"); return 0x6000u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager"); return 0x4000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::uint32_t resolved_name) override {
        Check(manager == 0x4000u && resolved_name ==
                  (name == 0u ? 0x6000u : name),
              "NPC lookup uses payload string or empty fallback");
        calls.emplace_back("find npc"); return npc;
    }
    void ClearAnimationFlag(std::uint32_t component,
                            std::uint32_t value) override {
        Check(component == (player == 0u ? npc : player) + 0x170u &&
                  value == 0u, "action component +0x170 flag zero");
        calls.emplace_back("clear");
    }
    std::uint32_t UpdatePlayerEffects(std::uint32_t object) override {
        Check(object == player, "player effect object");
        calls.emplace_back("player effects"); return 0x81u;
    }
    std::uint32_t UpdateNpcEffects(std::uint32_t object) override {
        Check(object == npc, "NPC effect object");
        calls.emplace_back("npc effects"); return 0x82u;
    }
};

void PlayerBranches() {
    Runtime found;
    Check(pal4::ScriptActionHoldRelease::Player(3, found) == 0x81u &&
              found.calls == std::vector<std::string>{
                  "player manager", "find player", "clear", "player effects"},
          "player flag clear precedes effect refresh");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptActionHoldRelease::Player(3, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "find player"},
          "missing player skips effects");
}

void NpcBranches() {
    for (const bool empty : {false, true}) {
        Runtime found;
        found.player = 0u;
        if (empty) found.name = 0u;
        const auto result = pal4::ScriptActionHoldRelease::Npc(0x5000u, found);
        auto expected = std::vector<std::string>{"name"};
        if (empty) expected.emplace_back("empty name");
        expected.insert(expected.end(), {"npc manager", "find npc",
                                         "clear", "npc effects"});
        Check(result == 0x82u && found.calls == expected,
              "NPC action release order and string fallback");
    }
    Runtime missing;
    missing.npc = 0u;
    Check(pal4::ScriptActionHoldRelease::Npc(0x5000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "npc manager", "find npc"},
          "missing NPC skips flag and effects");
}

}  // namespace

int main() {
    try {
        PlayerBranches();
        NpcBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
