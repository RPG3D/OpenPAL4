#include "pal4/script_party_membership.h"

#include "pal4/player_manager_leader_transition.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPartyMembershipRuntime {
    static constexpr std::array<std::uint32_t, 4> kPlayers{
        0x10000u, 0x11000u, 0u, 0x13000u};
    bool target_member = false;
    bool missing_target = false;
    bool fail_format = false;
    std::uint32_t window = 0x7000u;
    std::uint32_t freed_result = 0x5555u;
    std::uint32_t free_count = 0u;
    std::vector<std::uint32_t> freed_players;
    std::vector<std::string> calls;
    std::string hint_text = "party hint";

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == kPlayers[1] + 0xb08u) return target_member ? 1u : 0u;
        if (address == kPlayers[0] + 0xb08u ||
            address == kPlayers[3] + 0xb08u) return 1u;
        if (address == kPlayers[1] + 0x920u) return 7u;
        return 0u;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
        if (address == kPlayers[1] + 0xb08u) target_member = value != 0u;
    }
    std::uint32_t InventoryManager() override {
        calls.emplace_back("inventory"); return 0x8000u;
    }
    std::uint32_t FindItem(std::uint32_t collection,
                           std::uint32_t key,
                           std::uint32_t category) override {
        Check(collection == 0x807cu && key == 7u && category == 7u,
              "activation checks equipped item");
        calls.emplace_back("find-item"); return 0x9000u;
    }
    std::uint8_t CheckPlayerCondition(std::uint32_t item) override {
        Check(item == 0x9000u, "equipped item condition operand");
        calls.emplace_back("condition"); return 1u;
    }

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return 0xa000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0xa000u && index < 4u, "player lookup operands");
        calls.emplace_back("player:" + std::to_string(index));
        if (index == 1u && missing_target) return 0u;
        return kPlayers[index];
    }
    void SortPlayers(std::vector<std::uint32_t>& players,
                     std::uint32_t comparator) override {
        Check(comparator == pal4::PlayerManagerLeaderTransition::kComparator,
              "reuses established player-state comparator");
        calls.emplace_back("sort");
        std::sort(players.begin(), players.end(),
                  [](std::uint32_t left, std::uint32_t right) {
                      return left > right;
                  });
    }
    void SetPartyOrder(std::uint32_t player_state,
                       std::uint32_t position) override {
        calls.emplace_back("order:" + std::to_string(player_state) +
                           ":" + std::to_string(position));
    }
    std::uint32_t FreePartyList(
        const std::vector<std::uint32_t>& players) noexcept override {
        ++free_count;
        freed_players = players;
        calls.emplace_back("free");
        return freed_result;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui"); return 0xb000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t frames) override {
        Check(frames == 0xb000u, "UI manager passed to hint factory");
        calls.emplace_back("window"); return window;
    }
    void InitializeHintText(std::uint32_t buffer) override {
        Check(buffer <= 1u, "two text buffers");
        calls.emplace_back("init-text:" + std::to_string(buffer));
    }
    std::string_view PlayerName(std::uint32_t player) override {
        Check(player == kPlayers[1], "name at player +0x9fc");
        calls.emplace_back("name"); return "Yun";
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("db"); return 0xc000u;
    }
    std::string_view GameString(std::uint32_t table,
                                std::uint32_t id) override {
        Check(table == 0xc034u && (id == 0xfdu || id == 0xfeu),
              "party hint game-string table and ID");
        calls.emplace_back("string:" + std::to_string(id));
        return id == 0xfdu ? "Join %s" : "Leave %s";
    }
    void FormatHintText(std::string_view format,
                        std::string_view name) override {
        Check((format == "Join %s" || format == "Leave %s") &&
              name == "Yun", "hint formatting operands");
        calls.emplace_back("format");
        if (fail_format) throw std::runtime_error("format failed");
    }
    void AppendHintText() override { calls.emplace_back("append"); }
    std::string_view HintText() override {
        calls.emplace_back("text"); return hint_text;
    }
    void ShowHint(std::uint32_t hint, std::string_view text,
                  float x, float y, float scale, std::uint32_t mode,
                  std::uint32_t enabled) override {
        Check(hint == window && text == hint_text && x == 0.5f &&
              y == 0.15f && scale == 1.0f && mode == 0u && enabled == 1u,
              "party hint presentation");
        calls.emplace_back("show");
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio"); return 0xd000u;
    }
    void PlaySound(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0xd000u && flag == 0x80u && mode == 1u &&
              (path == "gamedata\\PALSound\\WA41.wav" ||
               path == "gamedata\\PALSound\\WA42.wav"),
              "party audio operands");
        calls.emplace_back("sound:" + std::string(path));
    }
    void CleanupHintText(std::uint32_t buffer) noexcept override {
        calls.emplace_back("cleanup:" + std::to_string(buffer));
    }
};

std::size_t Position(const std::vector<std::string>& calls,
                     const std::string& wanted) {
    const auto it = std::find(calls.begin(), calls.end(), wanted);
    Check(it != calls.end(), "missing expected call");
    return static_cast<std::size_t>(it - calls.begin());
}

void Gates() {
    Runtime missing;
    missing.missing_target = true;
    Check(pal4::ScriptPartyMembership::Add(1u, 0u, missing) == 0u &&
          missing.free_count == 0u, "missing player returns null");

    Runtime already_in;
    already_in.target_member = true;
    Check(pal4::ScriptPartyMembership::Add(1u, 0u, already_in) ==
              Runtime::kPlayers[1] && already_in.free_count == 0u,
          "already joined player returns original pointer");

    Runtime already_out;
    Check(pal4::ScriptPartyMembership::Remove(1u, 0u, already_out) ==
              Runtime::kPlayers[1] && already_out.free_count == 0u,
          "already removed player returns original pointer");
}

void AddAndRemove() {
    Runtime added;
    Check(pal4::ScriptPartyMembership::Add(1u, 0u, added) ==
              added.freed_result && added.target_member &&
          added.free_count == 1u &&
          added.freed_players == std::vector<std::uint32_t>{
              Runtime::kPlayers[3], Runtime::kPlayers[0],
              Runtime::kPlayers[1]},
          "add sorts current members before appending target");
    Check(Position(added.calls, "sort") <
              Position(added.calls, "write:72456:1") &&
          Position(added.calls, "condition") <
              Position(added.calls, "write:72456:1") &&
          Position(added.calls, "order:" +
              std::to_string(Runtime::kPlayers[3] + 0x654u) + ":0") <
              Position(added.calls, "free"),
          "activation and ordering happen after existing-member sort");

    Runtime removed;
    removed.target_member = true;
    Check(pal4::ScriptPartyMembership::Remove(1u, 0u, removed) ==
              removed.freed_result && !removed.target_member &&
          removed.free_count == 1u &&
          removed.freed_players == std::vector<std::uint32_t>{
              Runtime::kPlayers[3], Runtime::kPlayers[0]},
          "remove clears membership before four-player scan");
    Check(Position(removed.calls, "write:72456:0") <
              Position(removed.calls, "player:0") &&
          Position(removed.calls, "sort") <
          Position(removed.calls, "order:" +
              std::to_string(Runtime::kPlayers[3] + 0x654u) + ":0"),
          "removal update, scan, sort and order sequence");
}

void HintsAndCleanup() {
    Runtime no_window;
    no_window.window = 0u;
    Check(pal4::ScriptPartyMembership::Add(1u, 0x80u, no_window) ==
              no_window.freed_result && no_window.target_member &&
          Position(no_window.calls, "window") <
              Position(no_window.calls, "free"),
          "hint failure does not undo party mutation");

    Runtime added;
    (void)pal4::ScriptPartyMembership::Add(1u, 0x80u, added);
    Check(Position(added.calls, "string:253") <
              Position(added.calls, "sound:gamedata\\PALSound\\WA41.wav") &&
          Position(added.calls, "cleanup:1") <
              Position(added.calls, "cleanup:0") &&
          Position(added.calls, "cleanup:0") <
              Position(added.calls, "free"),
          "join hint, sound and reverse text cleanup");

    Runtime removed;
    removed.target_member = true;
    (void)pal4::ScriptPartyMembership::Remove(1u, 0x80u, removed);
    Check(Position(removed.calls, "string:254") <
              Position(removed.calls, "sound:gamedata\\PALSound\\WA42.wav"),
          "leave hint and sound selection");

    Runtime failure;
    failure.fail_format = true;
    try {
        (void)pal4::ScriptPartyMembership::Add(1u, 0x80u, failure);
        throw std::runtime_error("missing format failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "format failed",
              "format exception forwarded");
    }
    Check(failure.free_count == 1u &&
          Position(failure.calls, "cleanup:1") <
              Position(failure.calls, "cleanup:0") &&
          Position(failure.calls, "cleanup:0") <
              Position(failure.calls, "free"),
          "exception releases both texts and party vector");
}

}  // namespace

int main() {
    try {
        Gates();
        AddAndRemove();
        HintsAndCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
