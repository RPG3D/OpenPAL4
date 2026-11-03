#include "pal4/script_full_rage.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptFullRageRuntime {
    std::vector<std::string> calls;
    std::array<std::uint32_t, 4> active{};
    std::uint32_t acquired = 0u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager" + std::to_string(acquired));
        return 0x4000u + 0x100u * acquired++;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(index < 4u && manager == 0x4000u + index * 0x100u,
              "manager reacquisition and ascending player indexes");
        calls.emplace_back("player" + std::to_string(index));
        return 0x5000u + index * 0x100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address >= 0x5b08u && address <= 0x5e08u &&
                  (address - 0x5b08u) % 0x100u == 0u,
              "player active marker +0xb08");
        const auto index = (address - 0x5b08u) / 0x100u;
        calls.emplace_back("active" + std::to_string(index));
        return active[index];
    }
    std::uint32_t SetStat(std::uint32_t state,
                          std::uint32_t stat,
                          std::uint32_t value) override {
        Check(state >= 0x5654u && state <= 0x5954u &&
                  (state - 0x5654u) % 0x100u == 0u &&
                  stat == 8u && value == 100u,
              "rage stat virtual +0x44 operands");
        const auto index = (state - 0x5654u) / 0x100u;
        calls.emplace_back("set" + std::to_string(index));
        return 0xa000u + index;
    }
};

void InactiveAndActive() {
    Runtime none;
    Check(pal4::ScriptFullRage::Set(none) == 0x5300u &&
              none.acquired == 4u,
          "inactive last player handle is returned");
    Check(none.calls == std::vector<std::string>{
              "manager0", "player0", "active0", "manager1", "player1",
              "active1", "manager2", "player2", "active2", "manager3",
              "player3", "active3"},
          "all four slots visited without stat calls");

    Runtime mixed;
    mixed.active = {1u, 0u, 7u, 0u};
    Check(pal4::ScriptFullRage::Set(mixed) == 0x5300u &&
              mixed.calls == std::vector<std::string>{
                  "manager0", "player0", "active0", "set0", "manager1",
                  "player1", "active1", "manager2", "player2", "active2",
                  "set2", "manager3", "player3", "active3"},
          "only active players receive stat update");

    Runtime last_active;
    last_active.active = {0u, 0u, 0u, 1u};
    Check(pal4::ScriptFullRage::Set(last_active) == 0xa003u &&
              last_active.calls.back() == "set3",
          "last active stat result is forwarded");
}

}  // namespace

int main() {
    try {
        InactiveAndActive();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
