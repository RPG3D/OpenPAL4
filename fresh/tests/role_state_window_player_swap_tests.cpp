#include "pal4/role_state_window_button_up.h"
#include "pal4/role_state_window_members.h"
#include "pal4/role_state_window_player_swap.h"

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
                       pal4::IRoleStateWindowSwapRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::array<std::uint32_t, 4> players{
        0u, 0x10000u, 0u, 0x30000u};
    std::vector<std::uint32_t> prepared_members;
    std::vector<std::string> calls;
    std::uint32_t dialog_count = 0u;
    std::uint32_t sound_count = 0u;
    std::string warning = "swap denied";

    Runtime() {
        memory[0x10000u + 0xb08u] = 1u;
        memory[0x30000u + 0xb08u] = 1u;
        memory[0x10000u + 0xa48u] = 1u;
        memory[0x30000u + 0xa48u] = 3u;
        memory[0x9008u] = 0xa000u;
        memory[0xa118u] = 4u;
    }

    std::uint32_t PlayerManager() override { return 0x6000u; }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x6000u && index < 4u,
              "player lookup for role IDs and click count");
        calls.push_back("player:" + std::to_string(index));
        return players[index];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    void PreparePlayerData(std::uint32_t data,
                           std::uint32_t member_role) override {
        Check(data == 0x10654u || data == 0x30654u,
              "player data address for swapped roles");
        prepared_members.push_back(member_role);
        calls.emplace_back("prepare");
    }
    void BeginTextBuffer() override {}
    void EndTextBuffer() override {}
    void SetImage(std::uint32_t, std::uint32_t) override {}
    std::string ConvertToUtf8(std::uint32_t) override { return "role"; }
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
        Check(false, "level 99 needs no threshold");
        return 0;
    }
    void SetText(std::uint32_t, const std::string&) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetWidth(std::uint32_t, float) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    void ClearImage(std::uint32_t) override {}

    std::string WarningText(std::uint32_t trigger) override {
        Check(trigger == 0xe4u, "swap restriction trigger ID");
        calls.emplace_back("warning-text");
        return warning;
    }
    std::uint32_t CreateCommonDialog() override {
        ++dialog_count;
        calls.emplace_back("create-dialog");
        return 0xd000u;
    }
    std::uint8_t ShowDialog(std::uint32_t dialog,
                            const std::string& text,
                            std::uint32_t x_bits,
                            std::uint32_t y_bits,
                            std::uint32_t style,
                            std::uint32_t option) override {
        Check(dialog == 0xd000u && text == warning &&
                  x_bits == 0x3f000000u &&
                  y_bits == 0x3f000000u &&
                  style == 4u && option == 0u,
              "warning dialog text, center coordinates and options");
        calls.emplace_back("show-dialog");
        return 9u;
    }
    std::uint8_t PlaySound(std::string_view name,
                           std::uint32_t option) override {
        Check(name == "wa12.wav" && option == 1u,
              "swap sound resource and option");
        ++sound_count;
        calls.emplace_back("sound");
        return 7u;
    }
};

pal4::RoleStateWindowState MakeState() {
    pal4::RoleStateWindowState state;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto base = pal4::RoleStateWindowMembers::Offset(i);
        state.Write(base, i);
        state.Write(base + 8u, i);
        for (std::uint32_t offset : {12u, 20u, 24u, 28u, 32u, 36u,
                                     40u, 48u, 52u, 56u, 60u, 64u,
                                     68u, 72u, 76u, 80u, 84u, 88u,
                                     92u, 96u, 100u})
            state.Write(base + offset, 0x5000u + i * 0x200u + offset);
        for (std::uint32_t portrait = 0u; portrait < 4u; ++portrait)
            state.Write(base + 120u + portrait * 4u,
                        0x7000u + portrait);
        for (std::uint32_t offset : {104u, 108u, 112u})
            state.Write(base + offset, 0x3f800000u);
    }
    return state;
}

void TestSameMemberReturnsOwnerLowByte() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::RoleStateWindowPlayerSwap::Handle(
              state, 0x7005u, 1u, 1u, runtime, runtime) == 5u &&
              runtime.calls.empty(),
          "same member returns low owner byte without services");
}

void TestUnrestrictedAndAllowedProperty() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::RoleStateWindowPlayerSwap::Handle(
              state, 0x7000u, 3u, 1u, runtime, runtime) == 7u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(3u)) == 1u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(1u)) == 3u &&
              runtime.prepared_members ==
                  std::vector<std::uint32_t>{3u, 1u} &&
              runtime.sound_count == 1u && runtime.dialog_count == 0u &&
              runtime.calls.back() == "sound",
          "unrestricted swap updates target first, selected second, then plays sound");

    state = MakeState();
    Runtime allowed;
    state.Write(592u, 0xc000u);
    allowed.memory[0xc034u] = 2u;
    Check(pal4::RoleStateWindowPlayerSwap::Handle(
              state, 0x7000u, 3u, 1u, allowed, allowed) == 7u &&
              allowed.sound_count == 1u && allowed.dialog_count == 0u,
          "scene property type two permits swap");
}

void TestRestrictedPropertyDialog() {
    auto state = MakeState();
    state.Write(592u, 0xc000u);
    Runtime runtime;
    runtime.memory[0xc034u] = 1u;
    Check(pal4::RoleStateWindowPlayerSwap::Handle(
              state, 0x7000u, 3u, 1u, runtime, runtime) == 9u &&
              runtime.dialog_count == 1u && runtime.sound_count == 0u &&
              runtime.prepared_members.empty() &&
              state.Read(pal4::RoleStateWindowMembers::Offset(3u)) == 3u,
          "restricted property shows warning without modifying roles");
    Check(runtime.calls[runtime.calls.size() - 3u] == "warning-text" &&
              runtime.calls[runtime.calls.size() - 2u] == "create-dialog" &&
              runtime.calls.back() == "show-dialog",
          "warning lookup precedes dialog creation and display");

    state = MakeState();
    state.Write(592u, 0xc000u);
    Runtime empty_warning;
    empty_warning.memory[0xc034u] = 0u;
    empty_warning.warning.clear();
    Check(pal4::RoleStateWindowPlayerSwap::Handle(
              state, 0x7000u, 3u, 1u,
              empty_warning, empty_warning) == 9u,
          "missing warning text uses empty string");
}

void TestClickAdapterComposesSwap() {
    auto state = MakeState();
    state.Write(pal4::RoleStateWindowMembers::Offset(1u) + 4u, 2u);
    Runtime runtime;
    Check(pal4::RoleStateWindowButtonUp::Handle(
              state, 0x7000u, 0x9000u, runtime, runtime) &&
              state.Read(pal4::RoleStateWindowMembers::Offset(3u)) == 1u &&
              state.Read(pal4::RoleStateWindowMembers::Offset(1u)) == 3u &&
              runtime.sound_count == 1u,
          "button event reaches reconstructed swap and player-info methods");
}

}  // namespace

int main() {
    TestSameMemberReturnsOwnerLowByte();
    TestUnrestrictedAndAllowedProperty();
    TestRestrictedPropertyDialog();
    TestClickAdapterComposesSwap();
}
