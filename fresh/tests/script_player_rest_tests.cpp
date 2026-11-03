#include "pal4/script_player_rest.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerRestRuntime {
    std::array<bool, 4> present{true, true, true, true};
    std::array<bool, 4> in_party{true, true, true, true};
    std::array<std::int32_t, 4> hp{10, 20, -30, 40};
    std::array<std::int32_t, 4> mp{1, 2, 3, 4};
    std::array<std::int32_t, 4> current_hp{};
    std::array<std::int32_t, 4> current_mp{};
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                            std::uint32_t index) override {
        Check(manager == 0x1000u && index < 4u, "player manager lookup");
        calls.emplace_back("player:" + std::to_string(index));
        return present[index] ? 0x2000u + index * 0x1000u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto index = (address - 0x2b08u) / 0x1000u;
        Check(index < 4u && address == 0x2b08u + index * 0x1000u,
              "party membership offset");
        calls.emplace_back("party:" + std::to_string(index));
        return in_party[index] ? 1u : 0u;
    }
    std::int32_t GetStat(std::uint32_t bank,
                         std::uint32_t slot) override {
        const auto index = (bank - 0x2654u) / 0x1000u;
        Check(index < 4u && bank == 0x2654u + index * 0x1000u &&
                  (slot == 0u || slot == 2u), "stat bank getter");
        calls.emplace_back("get:" + std::to_string(index) + ":" +
                           std::to_string(slot));
        return slot == 0u ? hp[index] : mp[index];
    }
    void SetStat(std::uint32_t bank, std::uint32_t slot,
                 std::int32_t value) override {
        const auto index = (bank - 0x2654u) / 0x1000u;
        Check(index < 4u && bank == 0x2654u + index * 0x1000u &&
                  (slot == 7u || slot == 9u), "stat bank setter");
        calls.emplace_back("set:" + std::to_string(index) + ":" +
                           std::to_string(slot));
        if (slot == 7u) current_hp[index] = value;
        else current_mp[index] = value;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio-manager");
        return 0x7000u;
    }
    std::uint32_t PlayMusic(std::uint32_t manager,
                            std::string_view path,
                            std::uint32_t mode) override {
        Check(manager == 0x7000u &&
                  path == "gamedata\\PALSound\\WJ063.wav" && mode == 1u,
              "rest audio path and mode");
        calls.emplace_back("music");
        return 0x1234u;
    }
};

void AllMembers() {
    Runtime runtime;
    Check(pal4::ScriptPlayerRest::RestoreParty(runtime) == 0x1234u,
          "audio result forwarded");
    Check(runtime.current_hp == runtime.hp &&
              runtime.current_mp == runtime.mp,
          "all party members restored from max stats");
    Check(runtime.calls.size() == 30u, "four complete member passes");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto start = static_cast<std::size_t>(index) * 7u;
        Check(runtime.calls[start] == "manager" &&
                  runtime.calls[start + 1u] ==
                      "player:" + std::to_string(index) &&
                  runtime.calls[start + 2u] ==
                      "party:" + std::to_string(index) &&
                  runtime.calls[start + 3u] ==
                      "get:" + std::to_string(index) + ":0" &&
                  runtime.calls[start + 4u] ==
                      "set:" + std::to_string(index) + ":7" &&
                  runtime.calls[start + 5u] ==
                      "get:" + std::to_string(index) + ":2" &&
                  runtime.calls[start + 6u] ==
                      "set:" + std::to_string(index) + ":9",
              "per-player manager, membership and stat order");
    }
    Check(runtime.calls[28] == "audio-manager" &&
              runtime.calls[29] == "music", "rest sound follows loop");
}

void MissingAndOutsideParty() {
    Runtime runtime;
    runtime.present = {false, true, true, false};
    runtime.in_party = {true, false, true, true};
    (void)pal4::ScriptPlayerRest::RestoreParty(runtime);
    Check(runtime.current_hp == std::array<std::int32_t, 4>{0, 0, -30, 0} &&
              runtime.current_mp == std::array<std::int32_t, 4>{0, 0, 3, 0},
          "only present party member restored");
    Check(runtime.calls == std::vector<std::string>{
              "manager", "player:0", "manager", "player:1", "party:1",
              "manager", "player:2", "party:2", "get:2:0", "set:2:7",
              "get:2:2", "set:2:9", "manager", "player:3",
              "audio-manager", "music"},
          "missing players skip membership, sound still plays");
}

}  // namespace

int main() {
    try {
        AllMembers();
        MissingAndOutsideParty();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
