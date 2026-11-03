#include "pal4/script_pet_visibility.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPetVisibilityRuntime {
    std::vector<std::string> calls;
    std::array<std::uint32_t, 2> managers{0x5000u, 0x6000u};
    std::array<std::uint32_t, 2> indexes{0u, 2u};
    std::uint32_t expected_enabled = 1u;
    std::uint32_t pet_count = 0u;
    std::uint32_t arena_count = 0u;
    std::uint32_t player_count = 0u;

    std::uint32_t PetManager() override {
        calls.emplace_back("pet-manager");
        return 0x1000u + 0x100u * pet_count++;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x2000u + 0x100u * arena_count++;
    }
    std::uint32_t PlayerManager() override {
        Check(player_count < managers.size(), "two player acquisitions");
        calls.emplace_back("player-manager");
        return managers[player_count++];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto i = player_count - 1u;
        Check(address == managers[i] + 0x2c50u,
              "current player index offset");
        calls.emplace_back("index");
        return indexes[i];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x1654u && value == expected_enabled,
              "pet visibility word and bool normalization");
        calls.emplace_back("enabled");
    }
    void RemovePet(std::uint32_t arena) override {
        Check(arena == 0x2000u, "remove from first arena");
        calls.emplace_back("remove");
    }
    std::uint32_t CleanupPet(std::uint32_t manager) override {
        Check(manager == 0x1100u, "cleanup on second pet manager");
        calls.emplace_back("cleanup");
        return 0xcafeu;
    }
    void CreatePet(std::uint32_t manager,
                   std::string_view kind, std::string_view id,
                   std::uint32_t mode) override {
        Check(manager == 0x1200u && kind == "PAL4Pet" &&
                  id == "339" && mode == 1u,
              "pet creation operands");
        calls.emplace_back("create");
    }
    void InitializePet(std::uint32_t manager,
                       std::uint32_t player) override {
        Check(manager == 0x1300u &&
                  player == managers[1] + indexes[1] * 0xb14u,
              "pet initialization uses second current player");
        calls.emplace_back("initialize");
    }
    void SetStateFlag(std::uint32_t manager,
                      std::uint32_t flag) override {
        Check(manager == 0x1400u && flag == 1u,
              "pet game-object state flag");
        calls.emplace_back("state");
    }
    std::uint32_t AddPet(std::uint32_t arena) override {
        Check(arena == 0x2100u, "add to second arena acquisition");
        calls.emplace_back("add");
        return 0xdeadbeefu;
    }
};

void Disabled() {
    Runtime runtime;
    runtime.expected_enabled = 0u;
    Check(pal4::ScriptPetVisibility::SetEnabled(0u, runtime) == 0xcafeu,
          "disabled path returns cleanup result");
    Check(runtime.calls == std::vector<std::string>{
              "pet-manager", "enabled", "arena-manager", "remove",
              "pet-manager", "cleanup"},
          "disabled path removes and cleans without player access");
}

void NoCreation() {
    Runtime other_player;
    other_player.indexes[0] = 1u;
    Check(pal4::ScriptPetVisibility::SetEnabled(1u, other_player) ==
                  other_player.managers[1],
          "nonzero first current index returns second manager");
    Check(other_player.calls == std::vector<std::string>{
              "pet-manager", "enabled", "arena-manager", "remove",
              "pet-manager", "cleanup", "player-manager", "index",
              "player-manager", "index"},
          "nonzero first index still reads second index");

    Runtime null_player;
    null_player.managers[1] = 0u;
    null_player.indexes[1] = 0u;
    Check(pal4::ScriptPetVisibility::SetEnabled(1u, null_player) == 0u &&
              null_player.calls == other_player.calls,
          "zero computed player skips pet creation");
}

void Creation() {
    Runtime runtime;
    Check(pal4::ScriptPetVisibility::SetEnabled(0xffu, runtime) ==
                  0xdeadbeefu,
          "enabled path forwards add result");
    Check(runtime.calls == std::vector<std::string>{
              "pet-manager", "enabled", "arena-manager", "remove",
              "pet-manager", "cleanup", "player-manager", "index",
              "player-manager", "index", "pet-manager", "create",
              "pet-manager", "initialize", "pet-manager", "state",
              "arena-manager", "add"},
          "pet reconstruction and add order");
}

}  // namespace

int main() {
    try {
        Disabled();
        NoCreation();
        Creation();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
