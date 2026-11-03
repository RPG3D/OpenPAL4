#include "pal4/pal_console_monster_commands.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleMonsterRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> monsters{0x5000u, 0x6000u};

    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena manager");
        return 0x3000u;
    }
    void LeaveCharacter(std::uint32_t arena,
                        std::uint32_t monster) override {
        Check(arena == 0x3000u &&
                  (monster == 0x5000u || monster == 0x6000u),
              "leave character operands");
        calls.emplace_back("leave " + std::to_string(monster));
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("monster manager");
        return 0x4000u;
    }
    void IterateAndCallback(std::uint32_t manager,
                            RemoveCallback callback,
                            std::uint32_t context) override {
        Check(manager == 0x4000u && context == 0u && callback != nullptr,
              "monster iteration operands");
        calls.emplace_back("iterate");
        for (const auto monster : monsters)
            Check(callback(monster, *this) == monster,
                  "remove callback preserves monster address");
    }
    std::int32_t Cleanup(std::uint32_t manager) override {
        Check(manager == 0x4000u, "cleanup manager");
        calls.emplace_back("cleanup");
        return 99;
    }
};

void Callback() {
    Runtime runtime;
    Check(pal4::PalConsoleMonsterCommands::RemoveCallback(0x5000u,
                                                           runtime) ==
              0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "arena manager", "leave 20480"},
          "remove callback leaves arena then returns input");
}

void KillAll() {
    Runtime runtime;
    Check(pal4::PalConsoleMonsterCommands::KillAll(runtime) == 99 &&
              runtime.calls == std::vector<std::string>{
                  "monster manager", "iterate", "arena manager",
                  "leave 20480", "arena manager", "leave 24576",
                  "monster manager", "cleanup"},
          "kill all iterates callbacks before second manager cleanup");
    Runtime empty;
    empty.monsters.clear();
    Check(pal4::PalConsoleMonsterCommands::KillAll(empty) == 99 &&
              empty.calls == std::vector<std::string>{
                  "monster manager", "iterate", "monster manager", "cleanup"},
          "empty collection still cleans manager");
}

}  // namespace

int main() {
    try {
        Callback();
        KillAll();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
