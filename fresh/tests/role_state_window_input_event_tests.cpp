#include "pal4/role_state_window_input_event.h"
#include "pal4/role_state_window_members.h"

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

struct Runtime final : pal4::IRoleStateWindowPlayerRuntime,
                       pal4::IRoleStateWindowInputRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::array<std::uint32_t, 4> players{
        0x10000u, 0x20000u, 0x30000u, 0x40000u};
    std::array<bool, 4> visible{true, false, true, false};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> resources;
    std::vector<std::uint32_t> prepared_roles;
    std::uint32_t input_manager_calls = 0u;
    std::uint32_t inventory_calls = 0u;
    std::uint32_t event_calls = 0u;
    std::uint32_t visibility_calls = 0u;

    Runtime() {
        memory[0x7000u + 0x1ee4u] = 1u;
        memory[0x7000u + 0x1ed8u] = 0x10fu;
        memory[0x7000u + 0x1ef4u] = 1u;
        memory[0x7000u + 0x1ee8u] = 0x21u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            memory[players[i] + 0xb08u] = (i % 2u == 0u) ? 1u : 0u;
            memory[players[i] + 0xa48u] = i;
        }
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    std::uint32_t InputManager() override {
        ++input_manager_calls;
        calls.emplace_back("input-manager");
        return 0x7000u;
    }
    std::uint32_t InventoryManager() override {
        ++inventory_calls;
        calls.emplace_back("inventory-manager");
        return 0x8000u;
    }
    void UpdateAndClampCounter(std::uint32_t inventory,
                               std::uint32_t value) override {
        Check(inventory == 0x8000u && value == 500u,
              "inventory counter clamped to 500");
        calls.emplace_back("inventory-counter");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame-manager");
        return 0x9000u;
    }
    void ProcessEvent(std::uint32_t tree, std::uint32_t command,
                      std::uint32_t first,
                      std::uint32_t second) override {
        Check(tree == 0x9104u && command == 0x8b13a0u &&
                  first == 1u && second == 0x4444u,
              "game-info event address, show flag and root window");
        ++event_calls;
        calls.emplace_back("event");
    }
    void UpdatePlayerResource(std::uint32_t data,
                              std::uint32_t field,
                              std::uint32_t value) override {
        Check(field == 8u && value == 100u,
              "player virtual update selector and amount");
        resources.push_back(data);
        calls.emplace_back("resource");
    }
    bool IsVisible(std::uint32_t window, bool recursive) override {
        Check(!recursive && window >= 0x500cu &&
                  (window - 0x500cu) % 0x100u == 0u,
              "member panel nonrecursive visibility");
        ++visibility_calls;
        calls.emplace_back("is-visible");
        return visible[(window - 0x500cu) / 0x100u];
    }

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return 0xa000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0xa000u && index < 4u,
              "player lookup for four slots and visible members");
        calls.push_back("player:" + std::to_string(index));
        return players[index];
    }
    void PreparePlayerData(std::uint32_t data,
                           std::uint32_t member_role) override {
        Check(data == players[member_role] + 0x654u,
              "refresh uses visible member's player index");
        prepared_roles.push_back(member_role);
        calls.emplace_back("prepare");
    }
    void BeginTextBuffer() override {}
    void EndTextBuffer() override {}
    void SetImage(std::uint32_t, std::uint32_t) override {}
    std::string ConvertToUtf8(std::uint32_t) override { return "name"; }
    std::int32_t ElementValue(std::uint32_t,
                              std::uint32_t) override { return 1; }
    std::int32_t BaseValue(std::uint32_t,
                           std::uint32_t) override { return 100; }
    std::int32_t CurrentValue(std::uint32_t,
                              std::uint32_t field) override {
        return field == 4u ? 99 : 10;
    }
    std::uint32_t TriggerText(std::uint32_t) override { return 0u; }
    std::int32_t NextLevelThreshold(std::uint32_t,
                                    std::int32_t) override {
        Check(false, "level 99 bypasses threshold");
        return 0;
    }
    void SetText(std::uint32_t, const std::string&) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetWidth(std::uint32_t, float) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    void ClearImage(std::uint32_t) override {}
};

pal4::RoleStateWindowState MakeState() {
    pal4::RoleStateWindowState state;
    state.Write(36u, 0x4444u);
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto base = pal4::RoleStateWindowMembers::Offset(i);
        state.Write(base, i);
        state.Write(base + 8u, i);
        for (std::uint32_t offset : {12u, 20u, 24u, 28u, 32u, 36u,
                                     40u, 48u, 52u, 56u, 60u, 64u,
                                     68u, 72u, 76u, 80u, 84u, 88u,
                                     92u, 96u, 100u})
            state.Write(base + offset, 0x5000u + i * 0x100u + offset);
        for (std::uint32_t portrait = 0u; portrait < 4u; ++portrait)
            state.Write(base + 120u + portrait * 4u,
                        0xb000u + portrait);
        for (std::uint32_t offset : {104u, 108u, 112u})
            state.Write(base + offset, 0x3f800000u);
    }
    return state;
}

void TestUnknownCodeAndInputGates() {
    auto state = MakeState();
    Runtime unknown;
    unknown.memory[0xc010u] = 99u;
    Check(!pal4::RoleStateWindowInputEvent::Handle(
              state, 0xc000u, unknown, unknown) &&
              unknown.input_manager_calls == 0u,
          "unknown input code returns false before manager access");

    const std::uint32_t offsets[]{0x1ee4u, 0x1ed8u,
                                  0x1ef4u, 0x1ee8u};
    for (std::uint32_t gate = 0u; gate < 4u; ++gate) {
        Runtime runtime;
        runtime.memory[0xc010u] = gate % 2u == 0u ? 5u : 10u;
        runtime.memory[0x7000u + offsets[gate]] =
            gate == 0u || gate == 2u ? 0xffffffffu : 0x10u;
        Check(pal4::RoleStateWindowInputEvent::Handle(
                  state, 0xc000u, runtime, runtime) &&
                  runtime.input_manager_calls == (gate < 2u ? 1u : 2u) &&
                  runtime.inventory_calls == 0u &&
                  runtime.resources.empty() &&
                  runtime.event_calls == 0u,
              "failed input gate returns true without work");
    }
}

void TestInventoryInput() {
    auto state = MakeState();
    Runtime runtime;
    runtime.memory[0xc010u] = 5u;
    Check(pal4::RoleStateWindowInputEvent::Handle(
              state, 0xc000u, runtime, runtime) &&
              runtime.input_manager_calls == 2u &&
              runtime.inventory_calls == 1u &&
              runtime.event_calls == 1u &&
              runtime.resources.empty() &&
              runtime.calls.back() == "event",
          "input 5 updates inventory counter before game-info event");
}

void TestPlayerResourceInput() {
    auto state = MakeState();
    Runtime runtime;
    runtime.memory[0xc010u] = 10u;
    Check(pal4::RoleStateWindowInputEvent::Handle(
              state, 0xc000u, runtime, runtime) &&
              runtime.input_manager_calls == 2u &&
              runtime.resources ==
                  std::vector<std::uint32_t>{0x10654u, 0x30654u} &&
              runtime.visibility_calls == 4u &&
              runtime.prepared_roles ==
                  std::vector<std::uint32_t>{0u, 2u} &&
              runtime.inventory_calls == 0u &&
              runtime.event_calls == 0u,
          "input 10 updates active players then refreshes visible panels");
    Check(state.Read(pal4::RoleStateWindowMembers::Offset(0u)) == 0u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(2u)) == 2u,
          "visible members retain refreshed player indices");
}

}  // namespace

int main() {
    TestUnknownCodeAndInputGates();
    TestInventoryInput();
    TestPlayerResourceInput();
}
