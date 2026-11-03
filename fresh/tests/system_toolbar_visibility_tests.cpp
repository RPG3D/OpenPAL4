#include "pal4/system_toolbar_visibility.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemToolbarVisibilityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::array<std::int32_t, 15> entries{};
    std::vector<std::string> calls;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t, std::uint32_t>> events;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::vector<std::uint32_t> entry_indices;
    std::uint32_t manager_calls = 0u;
    std::uint32_t game_calls = 0u;
    std::uint32_t player = 0x9000u;
    std::uint32_t last = 0u;

    Runtime() {
        words[0x5000u + 36u] = 0x6000u;
        for (std::uint32_t offset = 64u; offset <= 88u; offset += 4u) {
            words[0x5000u + offset] = 0x7000u + offset;
        }
        words[player + 0xb08u] = 1u;
        entries[8] = 1;
        entries[9] = 0;
        entries[10] = -2;
        entries[11] = 3;
        entries[13] = 0;
        entries[14] = 4;
    }
    void SetParent(const std::uint32_t frame,
                   const std::uint8_t visible,
                   const std::uint32_t parent) override {
        Check(frame == 0x5000u && parent == 0x8000u && visible <= 2u,
              "parent arguments");
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x4000u;
    }
    void DispatchEvent(const std::uint32_t tree,
                       const std::uint32_t key,
                       const std::uint32_t first,
                       const std::uint32_t second) override {
        events.emplace_back(tree, key, first, second);
        calls.emplace_back("event");
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t Game() override {
        ++game_calls;
        calls.emplace_back("game");
        return 0xa000u;
    }
    std::int32_t GameEntry(const std::uint32_t game,
                           const std::uint32_t index) override {
        Check(game == 0xa000u && index < entries.size(),
              "game entry arguments");
        calls.emplace_back("entry");
        entry_indices.push_back(index);
        return entries[index];
    }
    void SetEnabled(const std::uint32_t window,
                    const bool value) override {
        enabled.emplace_back(window, value);
        calls.emplace_back("enabled");
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return 0xb000u;
    }
    std::uint32_t PlayerByIndex(const std::uint32_t manager,
                                const std::uint32_t index) override {
        Check(manager == 0xb000u && index == 3u,
              "player lookup arguments");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t LastActiveMember(const std::uint32_t members) override {
        Check(members == 0x4134u, "active member collection");
        calls.emplace_back("last");
        return last;
    }
    std::uint32_t InsertMember(const std::uint32_t members,
                               const std::uint32_t member) override {
        Check(members == 0x4134u && member == 0x5028u,
              "insert member arguments");
        calls.emplace_back("insert");
        return 0xc001u;
    }
    std::uint32_t RemoveMember(const std::uint32_t members,
                               const std::uint32_t member) override {
        Check(members == 0x4134u && member == 0x5028u,
              "remove member arguments");
        calls.emplace_back("remove");
        return 0xc002u;
    }
};

void TestShow() {
    Runtime runtime;
    Check(pal4::SystemToolbarVisibility::Update(
              0x5000u, 2u, 0x8000u, runtime) == 0xc001u,
          "visible toolbar inserts member");
    Check(runtime.events == decltype(runtime.events){
              {0x4104u, 0x8b1394u, 2u, 0u},
              {0x4104u, 0x8b13a0u, 2u, 0x6000u},
              {0x4104u, 0x8b13a4u, 5u, 0u}},
          "event keys and exact two-word payloads");
    Check(runtime.enabled == std::vector<std::pair<std::uint32_t, bool>>{
              {0x7048u, true}, {0x7040u, false},
              {0x704cu, true}, {0x7044u, true},
              {0x7058u, true}, {0x7050u, false},
              {0x7054u, true}},
          "seven controls follow game/player state in IDA order");
    Check(runtime.entry_indices == std::vector<std::uint32_t>{
              8u, 9u, 10u, 11u, 13u, 14u} &&
              runtime.game_calls == 6u && runtime.manager_calls == 5u &&
              runtime.calls.front() == "parent" &&
              runtime.calls.back() == "insert",
          "game and frame-manager call counts");
}

void TestAlreadyActiveAndNoPlayer() {
    Runtime runtime;
    runtime.player = 0u;
    runtime.last = 0x5028u;
    Check(pal4::SystemToolbarVisibility::Update(
              0x5000u, 1u, 0x8000u, runtime) == 0x5028u &&
              runtime.enabled[4] == std::pair<std::uint32_t, bool>{
                  0x7058u, false} &&
              runtime.manager_calls == 4u &&
              runtime.calls.back() == "last",
          "null player disables forging and existing last member is reused");
}

void TestHide() {
    Runtime runtime;
    Check(pal4::SystemToolbarVisibility::Update(
              0x5000u, 0u, 0x8000u, runtime) == 0xc002u &&
              runtime.events == decltype(runtime.events){
                  {0x4104u, 0x8b1394u, 0u, 0u},
                  {0x4104u, 0x8b13a0u, 0u, 0x6000u}} &&
              runtime.enabled.empty() && runtime.entry_indices.empty() &&
              runtime.manager_calls == 3u &&
              runtime.calls.back() == "remove",
          "hide dispatches two events then removes active member");
}

}  // namespace

int main() {
    TestShow();
    TestAlreadyActiveAndNoPlayer();
    TestHide();
    return EXIT_SUCCESS;
}
