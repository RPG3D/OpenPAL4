#include "pal4/script_party_recovery.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPartyRecoveryRuntime {
    std::array<std::uint32_t, 4> members{0u, 1u, 1u, 0u};
    std::vector<std::string> calls;
    std::uint32_t next_index = 0u;
    std::uint32_t expected_max = 0u;
    std::uint32_t expected_current = 7u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x1000u && index == next_index++,
              "ordered player lookup");
        calls.emplace_back("player:" + std::to_string(index));
        return 0x2000u + index * 0xb14u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto index = next_index - 1u;
        Check(address == 0x2000u + index * 0xb14u + 0xb08u,
              "membership offset");
        calls.emplace_back("member:" + std::to_string(index));
        return members[index];
    }
    std::uint32_t ReadMaxStat(std::uint32_t stats,
                              std::uint32_t stat) override {
        const auto index = next_index - 1u;
        Check(stats == 0x2000u + index * 0xb14u + 0x654u &&
                  stat == expected_max, "max stat and component offset");
        calls.emplace_back("max:" + std::to_string(index));
        return 100u + index;
    }
    std::uint32_t WriteCurrentStat(std::uint32_t stats,
                                   std::uint32_t stat,
                                   std::uint32_t value) override {
        const auto index = next_index - 1u;
        Check(stats == 0x2000u + index * 0xb14u + 0x654u &&
                  stat == expected_current && value == 100u + index,
              "current stat copies max value");
        calls.emplace_back("write:" + std::to_string(index));
        return 0x8000u + index;
    }
};

void CheckRecovery(bool hp) {
    Runtime runtime;
    runtime.expected_max = hp ? 0u : 2u;
    runtime.expected_current = hp ? 7u : 9u;
    const auto result = hp ? pal4::ScriptPartyRecovery::RestoreHp(runtime)
                           : pal4::ScriptPartyRecovery::RestoreMp(runtime);
    Check(result == 0x2000u + 3u * 0xb14u && runtime.next_index == 4u &&
          runtime.calls == std::vector<std::string>{
              "manager", "player:0", "member:0",
              "manager", "player:1", "member:1", "max:1", "write:1",
              "manager", "player:2", "member:2", "max:2", "write:2",
              "manager", "player:3", "member:3"},
          "four lookups, member-only writes and final player return");

    Runtime all_members;
    all_members.expected_max = runtime.expected_max;
    all_members.expected_current = runtime.expected_current;
    all_members.members.fill(1u);
    const auto last_write = hp ? pal4::ScriptPartyRecovery::RestoreHp(all_members)
                               : pal4::ScriptPartyRecovery::RestoreMp(all_members);
    Check(last_write == 0x8003u,
          "last member write result is returned");
}

}  // namespace

int main() {
    try {
        CheckRecovery(true);
        CheckRecovery(false);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
