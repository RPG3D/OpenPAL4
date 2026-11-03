#include "pal4/script_entity_transform.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptEntityTransformRuntime {
    bool found = true;
    std::string expected_name = "npc";
    std::vector<std::string> calls;
    std::array<float, 3> position{};
    std::array<float, 3> rotation{};

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return 0x1000u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                             std::int32_t index) override {
        Check(manager == 0x1000u && index == -2, "signed player index");
        calls.emplace_back("player");
        return found ? 0x3000u : 0u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc-manager");
        return 0x2000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x2000u && name == expected_name,
              "NPC name lookup");
        calls.emplace_back("npc");
        return found ? 0x3000u : 0u;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x30a8u || address == 0x30a0u,
              "existing rotation field offsets");
        calls.emplace_back(address == 0x30a8u ? "read-z" : "read-x");
        return address == 0x30a8u ? 3.5f : -1.5f;
    }
    void SetPosition(std::uint32_t entity,
                     const std::array<float, 3>& value,
                     std::uint32_t flags) override {
        Check(entity == 0x3000u && flags == 0u,
              "position virtual call operands");
        position = value;
        calls.emplace_back("position");
    }
    std::uint32_t SetRotation(std::uint32_t entity,
                              const std::array<float, 3>& value,
                              std::uint32_t flags) override {
        Check(entity == 0x3000u && flags == 0u,
              "rotation virtual call operands");
        rotation = value;
        calls.emplace_back("rotation");
        return 0x4567u;
    }
};

void PlayerPaths() {
    Runtime found;
    Check(pal4::ScriptEntityTransform::PlayerByIndex(
              -2, 1.0f, 2.0f, 3.0f, 4.0f, found) == 0x4567u,
          "player transform forwards rotation result");
    Check(found.position == std::array<float, 3>{1.0f, 2.0f, 3.0f} &&
              found.rotation == std::array<float, 3>{-1.5f, 4.0f, 3.5f},
          "position and preserved X/Z rotation");
    Check(found.calls == std::vector<std::string>{
              "player-manager", "player", "read-z", "read-x",
              "position", "rotation"}, "player call order");
    Runtime missing;
    missing.found = false;
    Check(pal4::ScriptEntityTransform::PlayerByIndex(
              -2, 1.0f, 2.0f, 3.0f, 4.0f, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player-manager", "player"}, "missing player skips writes");
}

void NpcPaths() {
    Runtime found;
    Check(pal4::ScriptEntityTransform::NpcByName(
              "npc", 5.0f, 6.0f, 7.0f, 8.0f, found) == 0x4567u &&
              found.position == std::array<float, 3>{5.0f, 6.0f, 7.0f} &&
              found.rotation == std::array<float, 3>{-1.5f, 8.0f, 3.5f},
          "NPC transform values");
    Check(found.calls == std::vector<std::string>{
              "npc-manager", "npc", "read-z", "read-x",
              "position", "rotation"}, "NPC call order");
    Runtime missing;
    missing.found = false;
    Check(pal4::ScriptEntityTransform::NpcByName(
              "npc", 1.0f, 2.0f, 3.0f, 4.0f, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc-manager", "npc"}, "missing NPC skips writes");
    Runtime empty_name;
    empty_name.expected_name = "";
    empty_name.found = false;
    Check(pal4::ScriptEntityTransform::NpcByName(
              nullptr, 1.0f, 2.0f, 3.0f, 4.0f, empty_name) == 0u,
          "null script name uses empty fallback");
}

}  // namespace

int main() {
    try {
        PlayerPaths();
        NpcPaths();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
