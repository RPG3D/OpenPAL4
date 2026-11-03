#include "pal4/script_game_event_subscriptions.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kOwner = 0x1000u;
struct ExpectedBinding {
    std::string_view name;
    std::uint32_t mode;
    std::uint32_t callback;
};

constexpr std::array<ExpectedBinding, 19> kExpected{{
    {"me_can_take_action", 2u, 0x54d670u},
    {"re_can_take_action", 2u, 0x54d670u},
    {"show_monster_tips", 2u, 0x54cfd0u},
    {"hide_monster_tips", 2u, 0x54da00u},
    {"re_moved_at_action_sequence", 2u, 0x54c9f0u},
    {"me_moved_at_action_sequence", 2u, 0x54ca30u},
    {"joint_attack_respond", 2u, 0x54cc60u},
    {"select_join_atk", 2u, 0x54ccb0u},
    {"combat_over", 1u, 0x54ca70u},
    {"combat_start", 1u, 0x54c9e0u},
    {"re_join_stage", 3u, 0x54c9e0u},
    {"me_join_stage", 3u, 0x54c9e0u},
    {"me_leave_stage", 3u, 0x54c9e0u},
    {"re_leave_stage", 3u, 0x54c9e0u},
    {"ae_become_dead", 3u, 0x54c9e0u},
    {"ae_become_alive", 3u, 0x54c9e0u},
    {"ae_start_action", 3u, 0x54da00u},
    {"ae_ui_select", 3u, 0x54d980u},
    {"ae_ui_unselect", 3u, 0x54d9d0u},
}};

struct Runtime final : pal4::IEventSubscriptionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> groups;
    std::vector<std::string> calls;
    std::uint32_t keys = 0u;
    std::uint32_t registrations = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t EventKey(std::string_view name) override {
        Check(keys < kExpected.size() && name == kExpected[keys].name,
              "event name and order");
        calls.emplace_back("key:" + std::to_string(keys));
        return 0x2000u + keys++;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x4000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(registrations < kExpected.size() && router == 0x4000u &&
                  owner == kOwner && key == 0x2000u + registrations &&
                  mode == kExpected[registrations].mode &&
                  callback == kExpected[registrations].callback,
              "registration mode, key, owner and callback");
        calls.emplace_back("register:" + std::to_string(registrations));
        return 0x3000u + registrations++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x4000u, "event router for group lookup");
        calls.emplace_back("group");
        return groups[key];
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        calls.emplace_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free:" + std::to_string(node));
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void RegisterAllBindings() {
    Runtime runtime;
    Check(pal4::ScriptGameEventSubscriptions::Register(kOwner, runtime) ==
              0x3012u && runtime.keys == 19u &&
              runtime.registrations == 19u,
          "19 entries and final registration result");
    for (std::uint32_t index = 0u; index < kExpected.size(); ++index) {
        const auto slot = kOwner + 60u + index * 8u;
        Check(runtime.words[slot] == 0x3000u + index &&
                  runtime.words[slot + 4u] == 0x2000u + index,
              "handle and key slots");
    }
    const auto final_slot = kOwner + 60u + 18u * 8u;
    Check(Position(runtime, "key:0") < Position(runtime, "register:0") &&
              Position(runtime, "write:" + std::to_string(final_slot + 4u)) <
                  Position(runtime, "write:" + std::to_string(final_slot)),
          "last registration stores key before handle");
}

void CleanupBranchesAndRegistrationOrder() {
    Runtime runtime;
    const auto first = kOwner + 60u;
    const auto second = first + 8u;
    const auto last = first + 18u * 8u;
    runtime.words[first] = 0x4444u;
    runtime.words[first + 4u] = 0x2222u;
    runtime.words[second] = 0u;
    runtime.words[second + 4u] = 0xaaaa;
    runtime.words[last] = 0x5555u;
    runtime.words[last + 4u] = 0x3333u;
    runtime.groups[0x2222u] = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 2u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5004u] = 0x5200u;
    runtime.words[0x5100u] = 0x5200u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x7777u;
    runtime.words[0x5200u] = 0x5000u;
    runtime.words[0x5204u] = 0x5100u;
    runtime.words[0x520cu] = 0x4444u;
    Check(pal4::ScriptGameEventSubscriptions::Cleanup(kOwner, runtime) == 0u,
          "cleanup returns zero after fixed count");
    Check(runtime.words[0x5100u] == 0x5000u &&
              runtime.words[0x5004u] == 0x5100u &&
              runtime.words[0x6008u] == 1u,
          "matching callback unlinked and count decremented");
    Check(runtime.words[first] == 0u && runtime.words[first + 4u] == 0u &&
              runtime.words[second + 4u] == 0xaaaau &&
              runtime.words[last] == 0u && runtime.words[last + 4u] == 0u,
          "active slots clear, inactive slot untouched");
    Check(Position(runtime, "release:21000") <
              Position(runtime, "free:20992"),
          "release owned payload before freeing list node");

    Runtime replacement;
    replacement.words[first] = 0x4444u;
    replacement.words[first + 4u] = 0x2222u;
    replacement.groups[0x2222u] = 0x6000u;
    replacement.words[0x6004u] = 0x5000u;
    replacement.words[0x6008u] = 1u;
    replacement.words[0x5000u] = 0x5100u;
    replacement.words[0x5004u] = 0x5100u;
    replacement.words[0x5100u] = 0x5000u;
    replacement.words[0x5104u] = 0x5000u;
    replacement.words[0x510cu] = 0x4444u;
    (void)pal4::ScriptGameEventSubscriptions::Register(kOwner, replacement);
    Check(Position(replacement, "free:20736") <
              Position(replacement, "key:0"),
          "old subscription removed before new registrations");
}

}  // namespace

int main() {
    try {
        RegisterAllBindings();
        CleanupBranchesAndRegistrationOrder();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
