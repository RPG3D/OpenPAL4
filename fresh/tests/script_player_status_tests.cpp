#include "pal4/script_player_status.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerStatusRuntime {
    std::uint32_t manager = 0x10000u;
    std::uint32_t leader_state = 2u;
    std::uint32_t player_state = 3u;
    std::uint32_t player = 0x21000u;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return manager;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == manager + 0x2c50u) return 1u;
        if (address == manager + 0xb14u + 0xa48u) return leader_state;
        if (address == player + 0xa48u) return player_state;
        throw std::runtime_error("wrong player word address");
    }
    std::uint32_t PlayerByIndex(std::uint32_t selected_manager,
                                std::int32_t index) override {
        Check(selected_manager == manager && index == 1,
              "level player lookup operands");
        calls.emplace_back("player"); return player;
    }
    void WriteLevel(std::uint32_t state, std::uint32_t stat,
                    std::int32_t level) override {
        Check(state == player + 0x654u && stat == 4u &&
              level > 0 && level < 100,
              "level stat virtual setter operands");
        calls.emplace_back("write:" + std::to_string(level));
    }
    void RefreshPlayerState(std::uint32_t state,
                            std::int32_t level,
                            std::uint32_t current_state) override {
        Check(state == player + 0x654u && level > 0 && level < 100 &&
              current_state == player_state,
              "state refresh operands");
        calls.emplace_back("refresh");
    }
    std::uint32_t BuildMatchingRecords(
        std::uint32_t collection, std::int32_t level,
        std::uint32_t current_state) override {
        Check(collection == player + 0x938u && level > 0 && level < 100 &&
              current_state == player_state,
              "collection refresh operands");
        calls.emplace_back("build"); return 0x7000u;
    }
    std::uint32_t FreeRecords(std::uint32_t buffer) noexcept override {
        if (buffer != 0x7000u) std::terminate();
        calls.emplace_back("free"); return 0xbeefu;
    }
};

void LeaderBranches() {
    for (std::uint32_t state : {0u, 1u, 2u, 3u, 4u, 0xffffffffu}) {
        Runtime runtime;
        runtime.leader_state = state;
        Check(pal4::ScriptPlayerStatus::GetLeaderState(runtime) ==
                  (state <= 3u ? static_cast<std::int32_t>(state) : -1) &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read:" + std::to_string(0x12c50u),
                  "read:" + std::to_string(0x1155cu)},
              "leader state accepts only values zero through three");
    }
    Runtime zero;
    zero.manager = 0u - 0xb14u;
    Check(pal4::ScriptPlayerStatus::GetLeaderState(zero) == -1 &&
          zero.calls.size() == 2u,
          "zero current-player address skips state read");
}

void LevelBranches() {
    Runtime negative;
    Check(pal4::ScriptPlayerStatus::SetLevel(-2, 50, negative) ==
              static_cast<std::uint32_t>(-2) && negative.calls.empty(),
          "negative player index returns original argument");

    Runtime absent;
    absent.player = 0u;
    Check(pal4::ScriptPlayerStatus::SetLevel(1, 50, absent) == 0u &&
          absent.calls == std::vector<std::string>{"manager", "player"},
          "missing player has no stat or collection effects");

    for (const auto level : {0, 100, -1, 101}) {
        Runtime invalid;
        Check(pal4::ScriptPlayerStatus::SetLevel(1, level, invalid) ==
                  invalid.player &&
              invalid.calls == std::vector<std::string>{"manager", "player"},
              "only levels 1 through 99 are accepted");
    }

    for (const auto level : {1, 99}) {
        Runtime valid;
        Check(pal4::ScriptPlayerStatus::SetLevel(1, level, valid) ==
                  0xbeefu &&
              valid.calls == std::vector<std::string>{
                  "manager", "player", "write:" + std::to_string(level),
                  "read:" + std::to_string(valid.player + 0xa48u),
                  "refresh", "read:" +
                  std::to_string(valid.player + 0xa48u), "build", "free"},
              "level writes stat, refreshes state, rebuilds and frees records");
    }
}

}  // namespace

int main() {
    try {
        LeaderBranches();
        LevelBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
