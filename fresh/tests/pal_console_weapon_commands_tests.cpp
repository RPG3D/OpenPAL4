#include "pal4/pal_console_weapon_commands.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleWeaponRuntime {
    std::vector<std::string> calls;
    std::uint32_t manager = 0x1000u;
    std::uint32_t index = 2u;
    std::uint32_t state = 1u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == manager + 0x2c50u) {
            calls.emplace_back("index");
            return index;
        }
        Check(address == manager + 2836u * index + 0x1f0u,
              "player state field");
        calls.emplace_back("state");
        return state;
    }
    std::int32_t ApplyEquipmentVisuals(std::uint32_t player) override {
        Check(player == manager + 2836u * index,
              "equipment visuals player");
        calls.emplace_back("equip");
        return 41;
    }
    std::int32_t ClearWeaponEffects(std::uint32_t player) override {
        Check(player == manager + 2836u * index,
              "clear weapon player");
        calls.emplace_back("clear");
        return 43;
    }
};

using Command = std::int32_t (*)(pal4::IPalConsoleWeaponRuntime&);

void BothCommands() {
    for (const auto [command, call, result] :
         {std::tuple<Command, const char*, int>{
              &pal4::PalConsoleWeaponCommands::TakeOn, "equip", 41},
          std::tuple<Command, const char*, int>{
              &pal4::PalConsoleWeaponCommands::TakeOff, "clear", 43}}) {
        Runtime active;
        Check(command(active) == result &&
                  active.calls == std::vector<std::string>{
                      "manager", "index", "state", call},
              "active player with nonzero state calls weapon operation");
        Runtime hidden;
        hidden.state = 0u;
        Check(command(hidden) == 0 &&
                  hidden.calls == std::vector<std::string>{
                      "manager", "index", "state"},
              "zero player state skips weapon operation");
        Runtime absent;
        absent.manager = 0u;
        absent.index = 0u;
        Check(command(absent) == 0 &&
                  absent.calls ==
                      std::vector<std::string>{"manager", "index"},
              "zero computed player skips state read");
    }
}

}  // namespace

int main() {
    try {
        BothCommands();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
