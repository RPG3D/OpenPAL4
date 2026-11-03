#include "pal4/script_player_attribute_sync.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerAttributeSyncRuntime {
    std::int32_t missing_index = -1;
    std::uint32_t manager_calls = 0u;
    std::array<std::uint32_t, 4> hp{};
    std::array<std::uint32_t, 4> mp{};
    std::vector<std::string> calls;
    std::vector<double> converted;

    std::uint32_t PlayerManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x9000u && index >= 0 && index < 4,
              "player manager and signed index");
        calls.emplace_back("player:" + std::to_string(index));
        if (index == missing_index) return 0u;
        return 0x10000u + static_cast<std::uint32_t>(index) * 0x1000u;
    }
    std::int32_t ReadMaxStat(std::uint32_t state,
                             std::uint32_t stat) override {
        Check(state >= 0x10654u && state <= 0x13654u &&
              (state - 0x10654u) % 0x1000u == 0u &&
              (stat == 0u || stat == 2u),
              "player +0x654 getter virtual slot and stat ID");
        calls.emplace_back("read:" + std::to_string(stat));
        return stat == 0u ? 101 : 47;
    }
    std::uint32_t Ftol(double value) override {
        calls.emplace_back("ftol");
        converted.push_back(value);
        return static_cast<std::uint32_t>(
            static_cast<std::int32_t>(std::trunc(value)));
    }
    std::uint32_t WriteCurrentStat(
        std::uint32_t state, std::uint32_t stat,
        std::uint32_t value) override {
        Check((stat == 7u || stat == 9u) &&
              state >= 0x10654u && state <= 0x13654u,
              "player state setter virtual slot and current stat ID");
        const auto index = (state - 0x10654u) / 0x1000u;
        (stat == 7u ? hp[index] : mp[index]) = value;
        calls.emplace_back("write:" + std::to_string(stat));
        return state + stat;
    }
};

void SingleBranches() {
    Runtime hp;
    Check(pal4::ScriptPlayerAttributeSync::Set(1, 0, 50, hp) ==
              0x11654u + 7u && hp.hp[1] == 50u && hp.mp[1] == 0u &&
          hp.calls == std::vector<std::string>{
              "manager", "player:1", "read:0", "ftol", "write:7"},
          "one-player HP percentage path and return");

    Runtime mp;
    Check(pal4::ScriptPlayerAttributeSync::Set(2, 1, -25, mp) ==
              0x12654u + 9u &&
          static_cast<std::int32_t>(mp.mp[2]) == -11 &&
          mp.calls == std::vector<std::string>{
              "manager", "player:2", "read:2", "ftol", "write:9"},
          "one-player signed MP percentage truncates toward zero");

    Runtime invalid;
    Check(pal4::ScriptPlayerAttributeSync::Set(3, 4, 100, invalid) ==
              0x13000u &&
          invalid.calls == std::vector<std::string>{
              "manager", "player:3"},
          "unsupported attribute returns player without stat access");

    Runtime absent;
    absent.missing_index = 1;
    Check(pal4::ScriptPlayerAttributeSync::Set(1, 0, 100, absent) == 0u &&
          absent.calls == std::vector<std::string>{
              "manager", "player:1"},
          "missing player exits before stat update");
}

void BothStatRounding() {
    Runtime both;
    Check(pal4::ScriptPlayerAttributeSync::Set(0, -1, 100, both) ==
              0x10654u + 9u && both.hp[0] == 101u && both.mp[0] == 47u &&
          both.calls == std::vector<std::string>{
              "manager", "player:0", "read:0", "ftol", "write:7",
              "read:2", "ftol", "write:9"},
          "both-stat path stores percentage as float before scaling");
    Check(both.converted == std::vector<double>{101.0, 47.0},
          "float rounding restores exact 100 percent for both stats");

    Runtime one;
    (void)pal4::ScriptPlayerAttributeSync::Set(0, 0, 100, one);
    Check(one.converted[0] < 101.0 && one.hp[0] == 100u,
          "single-stat path retains extended percentage precision");
}

void AllPlayers() {
    Runtime all;
    Check(pal4::ScriptPlayerAttributeSync::Set(-3, 0, 50, all) ==
              0x13654u + 7u && all.manager_calls == 4u &&
          all.hp == std::array<std::uint32_t, 4>{50u, 50u, 50u, 50u},
          "negative player ID visits four players and forwards last result");

    Runtime invalid;
    Check(pal4::ScriptPlayerAttributeSync::Set(-1, 8, 100, invalid) ==
              0x13000u && invalid.manager_calls == 4u &&
          invalid.converted.empty(),
          "invalid attribute still scans all four players");

    Runtime missing;
    missing.missing_index = 2;
    Check(pal4::ScriptPlayerAttributeSync::Set(-1, 1, 50, missing) ==
              0u && missing.manager_calls == 3u &&
          missing.calls.back() == "player:2" &&
          missing.mp[0] == 23u && missing.mp[1] == 23u,
          "missing player stops all-player traversal after prior writes");
}

}  // namespace

int main() {
    try {
        SingleBranches();
        BothStatRounding();
        AllPlayers();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
