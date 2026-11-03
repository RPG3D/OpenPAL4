#include "pal4/role_state_window_button_up.h"
#include "pal4/role_state_window_members.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRoleStateWindowButtonRuntime {
    std::array<std::uint32_t, 4> players{
        0x10000u, 0x20000u, 0x30000u, 0x40000u};
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::vector<std::string> images;
    std::uint32_t swap_count = 0u;
    std::uint32_t swap_target = 99u;
    std::uint32_t swap_selected = 99u;
    std::uint32_t player_lookups = 0u;
    std::uint32_t event_reads = 0u;

    Runtime() {
        memory[0x10000u + 0xb08u] = 1u;
        memory[0x20000u + 0xb08u] = 1u;
        memory[0x30000u + 0xb08u] = 0u;
        memory[0x40000u + 0xb08u] = 0u;
        memory[0x9008u] = 0xa000u;
        memory[0xa118u] = 3u;
    }

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x6000u && index < 4u,
              "manager and four player indices");
        ++player_lookups;
        calls.push_back("player:" + std::to_string(index));
        return players[index];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x9008u || address == 0xa118u) ++event_reads;
        calls.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    void HandlePlayerSwap(pal4::RoleStateWindowState&,
                          std::uint32_t target_index,
                          std::uint32_t selected_index) override {
        ++swap_count;
        swap_target = target_index;
        swap_selected = selected_index;
        calls.emplace_back("swap");
    }
    void SetVisible(std::uint32_t, bool visible) override {
        Check(visible, "normal and hover artwork show background");
        calls.emplace_back("visible");
    }
    void SetImage(std::uint32_t, std::string_view set,
                  std::string_view image) override {
        Check(set == "kuang0", "role button image set");
        images.emplace_back(image);
        calls.emplace_back("image");
    }
    void ClearImage(std::uint32_t) override {
        calls.emplace_back("clear");
    }
};

pal4::RoleStateWindowState MakeState() {
    pal4::RoleStateWindowState state;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto base = pal4::RoleStateWindowMembers::Offset(i);
        state.Write(base, i);
        state.Write(base + 96u, 0x5000u + i * 0x100u);
        state.Write(base + 100u, 0x6000u + i * 0x100u);
    }
    return state;
}

void CheckAllNormal(const pal4::RoleStateWindowState& state,
                    const Runtime& runtime) {
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(state.Read(pal4::RoleStateWindowMembers::Offset(i) + 4u) == 0u,
              "all role modes reset to normal");
    Check(runtime.player_lookups == 4u,
          "all four player slots checked after artwork reset");
}

void TestFewerThanTwoPlayers() {
    auto state = MakeState();
    state.Write(pal4::RoleStateWindowMembers::Offset(1u) + 4u, 2u);
    Runtime runtime;
    runtime.players[1u] = 0u;
    Check(pal4::RoleStateWindowButtonUp::Handle(
              state, 0x9000u, runtime),
          "click always returns true");
    CheckAllNormal(state, runtime);
    Check(runtime.swap_count == 0u && runtime.event_reads == 0u &&
              runtime.images == std::vector<std::string>{
                  "renwuanniu1", "renwuanniub",
                  "renwuanniuc", "renwuanniud"},
          "one active player only restores four normal button images");
}

void TestHoverWithoutPreviousSelection() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::RoleStateWindowButtonUp::Handle(
              state, 0x9000u, runtime) &&
              state.Read(pal4::RoleStateWindowMembers::Offset(2u) + 4u) == 2u &&
              runtime.swap_count == 0u && runtime.event_reads == 2u,
          "role tag 3 selects third member's hovered state");
    Check(runtime.images.size() == 6u &&
              runtime.images[4u] == "huose" &&
              runtime.images[5u] == "renwuanniu3c3",
          "hover image follows four normal artwork resets");
}

void TestSelectedRoleSwap() {
    auto state = MakeState();
    state.Write(pal4::RoleStateWindowMembers::Offset(1u) + 4u, 2u);
    Runtime runtime;
    runtime.memory[0xa118u] = 4u;
    Check(pal4::RoleStateWindowButtonUp::Handle(
              state, 0x9000u, runtime) &&
              runtime.swap_count == 1u &&
              runtime.swap_target == 3u &&
              runtime.swap_selected == 1u,
          "selected role invokes swap with tagged target");
    CheckAllNormal(state, runtime);
    Check(std::count(runtime.images.begin(), runtime.images.end(),
                     "renwuanniub") == 2 &&
              runtime.calls.back() == "image",
          "previously selected button is restored again after swap");
}

void TestLastHoveredMemberWins() {
    auto state = MakeState();
    state.Write(pal4::RoleStateWindowMembers::Offset(0u) + 4u, 2u);
    state.Write(pal4::RoleStateWindowMembers::Offset(2u) + 4u, 2u);
    Runtime runtime;
    Check(pal4::RoleStateWindowButtonUp::Handle(
              state, 0x9000u, runtime) &&
              runtime.swap_selected == 2u,
          "loop keeps last member in hovered state");
}

}  // namespace

int main() {
    TestFewerThanTwoPlayers();
    TestHoverWithoutPreviousSelection();
    TestSelectedRoleSwap();
    TestLastHoveredMemberWins();
}
