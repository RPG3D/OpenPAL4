#include "pal4/role_state_window_events.h"
#include "pal4/role_state_window_members.h"

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

struct Runtime final : pal4::IRoleStateWindowEventRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::vector<std::string> images;
    std::vector<bool> visible_results{true, true};
    std::uint32_t active_count = 2u;
    std::uint32_t close_count = 0u;
    std::uint32_t visibility_queries = 0u;
    std::uint32_t mouse_window = 0u;
    std::uint32_t player_manager_calls = 0u;

    void Close(std::uint32_t owner) override {
        Check(owner == 0x7000u, "close uses owner before event subobject");
        ++close_count;
        calls.emplace_back("close");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame-manager");
        return 0x8000u;
    }
    void ProcessEvent(std::uint32_t tree, std::uint32_t command,
                      std::uint32_t first,
                      std::uint32_t second) override {
        Check(tree == 0x8104u && command == 0x8a80acu &&
                  first == 1u && second == 0u,
              "close event tree, callback and payload");
        calls.emplace_back("close-event");
    }
    std::uint32_t PlayerManager() override {
        ++player_manager_calls;
        calls.emplace_back("player-manager");
        return 0x9000u;
    }
    std::uint32_t ActivePlayerCount(std::uint32_t manager) override {
        Check(manager == 0x9000u, "active player count manager");
        calls.emplace_back("active-count");
        return active_count;
    }
    void HandleMouseEvent(std::uint32_t window) override {
        mouse_window = window;
        calls.emplace_back("mouse-event");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0xa010u, "event code read at +0x10");
        calls.emplace_back("event-code");
        return memory[address];
    }
    std::uint32_t CapturedWindow() override { return 0u; }
    std::uint32_t RootWindow() override { return 0u; }
    std::uint32_t ChildAtPosition(std::uint32_t,
                                  std::uint32_t) override { return 0u; }
    bool IsVisible(std::uint32_t window, bool recursive) override {
        Check(!recursive && window >= 0x5000u &&
                  (window - 0x5000u) % 0x100u == 0u,
              "next role panel nonrecursive visibility query");
        calls.emplace_back("is-visible");
        const auto result = visible_results[visibility_queries];
        ++visibility_queries;
        return result;
    }
    void SetVisible(std::uint32_t, bool visible) override {
        Check(visible, "role button background shown");
        calls.emplace_back("show-background");
    }
    void SetImage(std::uint32_t, std::string_view set,
                  std::string_view name) override {
        Check(set == "kuang0", "role image set");
        images.emplace_back(name);
        calls.emplace_back("image");
    }
    void ClearImage(std::uint32_t) override {
        calls.emplace_back("clear-image");
    }
};

pal4::RoleStateWindowState MakeState(std::uint32_t index = 0u) {
    pal4::RoleStateWindowState state;
    state.Write(588u, index);
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto base = pal4::RoleStateWindowMembers::Offset(i);
        state.Write(base, i);
        state.Write(base + 12u, 0x5000u + i * 0x100u);
        state.Write(base + 96u, 0x6000u + i * 0x100u);
        state.Write(base + 100u, 0x7000u + i * 0x100u);
    }
    return state;
}

void TestCloseBypassesPlayerGate() {
    auto state = MakeState();
    Runtime runtime;
    runtime.active_count = 0u;
    runtime.memory[0xa010u] = 1u;
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, runtime) &&
              runtime.close_count == 1u &&
              runtime.player_manager_calls == 0u &&
              runtime.calls == std::vector<std::string>{
                  "event-code", "close", "frame-manager", "close-event"},
          "close directly dispatches UI event without player count check");
}

void TestInsufficientPlayersAndUnknownCode() {
    auto state = MakeState();
    Runtime runtime;
    runtime.active_count = 1u;
    runtime.memory[0xa010u] = 203u;
    Check(!pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, runtime) &&
              runtime.images.empty() && state.Read(588u) == 0u,
          "less than two players rejects navigation");
    runtime.active_count = 0xffffffffu;
    Check(!pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, runtime),
          "player count comparison is signed");
    runtime.active_count = 2u;
    runtime.memory[0xa010u] = 999u;
    Check(!pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, runtime),
          "unrecognized event returns false");
}

void TestPreviousRoleAndHoverPreservation() {
    auto state = MakeState(2u);
    state.Write(pal4::RoleStateWindowMembers::Offset(3u) + 4u, 2u);
    Runtime runtime;
    runtime.memory[0xa010u] = 203u;
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, runtime) &&
              state.Read(588u) == 1u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(1u) + 4u) == 1u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(3u) + 4u) == 2u &&
              runtime.images.size() == 5u,
          "left navigation resets unhovered buttons and selects prior role");
    Check(runtime.images.back() == "renwuanniu2b2",
          "left navigation selects second role image");

    state = MakeState(0u);
    Runtime at_first;
    at_first.memory[0xa010u] = 203u;
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, at_first) &&
              state.Read(588u) == 0u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(0u) + 4u) == 1u,
          "left navigation clamps at first member");
}

void TestNextRoleVisibilityBranches() {
    Runtime at_last;
    at_last.memory[0xa010u] = 205u;
    auto state = MakeState(3u);
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, at_last) &&
              at_last.visibility_queries == 0u && state.Read(588u) == 3u,
          "right navigation at fourth role does not query beyond array");

    Runtime hidden;
    hidden.memory[0xa010u] = 205u;
    hidden.visible_results = {false};
    state = MakeState(1u);
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, hidden) &&
              hidden.visibility_queries == 1u &&
              hidden.images.empty() && state.Read(588u) == 1u,
          "hidden next role returns after first visibility query");

    Runtime changed_visibility;
    changed_visibility.memory[0xa010u] = 205u;
    changed_visibility.visible_results = {true, false};
    state = MakeState(1u);
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, changed_visibility) &&
              changed_visibility.visibility_queries == 2u &&
              changed_visibility.images.empty() && state.Read(588u) == 1u,
          "second visibility query can independently prevent navigation");

    Runtime visible;
    visible.memory[0xa010u] = 205u;
    state = MakeState(1u);
    state.Write(pal4::RoleStateWindowMembers::Offset(0u) + 4u, 2u);
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, visible) &&
              visible.visibility_queries == 2u && state.Read(588u) == 2u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(2u) + 4u) == 1u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(0u) + 4u) == 2u &&
              visible.images.back() == "renwuanniu2c2",
          "visible next role advances selection while preserving hover");
}

void TestCurrentPanelMouseEvent() {
    auto state = MakeState(2u);
    Runtime runtime;
    runtime.memory[0xa010u] = 28u;
    Check(pal4::RoleStateWindowEvents::OnEvent(
              state, 0x7000u, 0xa000u, runtime) &&
              runtime.mouse_window == 0x5200u &&
              runtime.calls.back() == "mouse-event",
          "event 28 forwards current role panel pointer");
}

void TestButtonImageRefresh() {
    auto state = MakeState();
    state.Write(pal4::RoleStateWindowMembers::Offset(1u) + 4u, 1u);
    state.Write(pal4::RoleStateWindowMembers::Offset(2u) + 4u, 2u);
    state.Write(pal4::RoleStateWindowMembers::Offset(3u) + 4u, 1u);
    Runtime runtime;
    Check(pal4::RoleStateWindowEvents::UpdateButtonImages(
              state, runtime) &&
              state.Read(pal4::RoleStateWindowMembers::Offset(1u) + 4u) == 0u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(2u) + 4u) == 2u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(3u) + 4u) == 0u &&
              runtime.images ==
                  std::vector<std::string>{"renwuanniub", "renwuanniud"},
          "mouse-move callback resets only selected state one buttons");
}

}  // namespace

int main() {
    TestCloseBypassesPlayerGate();
    TestInsufficientPlayersAndUnknownCode();
    TestPreviousRoleAndHoverPreservation();
    TestNextRoleVisibilityBranches();
    TestCurrentPanelMouseEvent();
    TestButtonImageRefresh();
}
