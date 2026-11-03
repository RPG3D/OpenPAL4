#include "pal4/combat_console_event_subscriptions.h"

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

constexpr std::uint32_t kConsole = 0x1000u;

struct ExpectedBinding {
    std::string_view name;
    std::uint32_t mode;
    std::uint32_t callback;
};

constexpr std::array<ExpectedBinding, 23> kExpected{{
    {"ae_physical_attack", 2u, 0x57b8d0u},
    {"ae_critical_attack", 2u, 0x57b8d0u},
    {"ae_fend_offed", 2u, 0x57b8d0u},
    {"ae_attack_missed", 2u, 0x540020u},
    {"ae_fleeing", 2u, 0x540170u},
    {"ae_revive", 2u, 0x57b8d0u},
    {"ae_flee_fail", 2u, 0x5401f0u},
    {"ae_protect_action", 2u, 0x540100u},
    {"ae_joint_attack", 2u, 0x57b8d0u},
    {"ae_react_attack", 2u, 0x5408b0u},
    {"ae_defende", 2u, 0x57b8d0u},
    {"re_attr_int_ext_changed", 2u, 0x5404b0u},
    {"me_attr_int_ext_changed", 2u, 0x5404b0u},
    {"ae_cast_magic", 2u, 0x540360u},
    {"ae_use_property", 2u, 0x5403d0u},
    {"ae_stunt", 2u, 0x540440u},
    {"ae_immunity_state_a", 3u, 0x540270u},
    {"ae_immunity_state_b", 3u, 0x540270u},
    {"ae_immunity_skill", 3u, 0x540270u},
    {"ae_immunity_normal", 3u, 0x540270u},
    {"add_picked_money", 2u, 0x540b40u},
    {"add_picked_property", 2u, 0x5409a0u},
    {"pick_up_fail", 2u, 0x540d20u},
}};

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> groups;
    std::vector<std::string> calls;
    std::uint32_t key_calls = 0u;
    std::uint32_t registrations = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t EventKey(std::string_view name) override {
        Check(key_calls < kExpected.size() &&
                  name == kExpected[key_calls].name,
              "event registration name/order");
        calls.emplace_back("key:" + std::to_string(key_calls));
        return 0x2000u + key_calls++;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x4000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(registrations < kExpected.size() &&
                  router == 0x4000u && owner == kConsole &&
                  key == 0x2000u + registrations &&
                  mode == kExpected[registrations].mode &&
                  callback == kExpected[registrations].callback,
              "event mode, callback and owner");
        calls.emplace_back("register:" +
                           std::to_string(registrations));
        return 0x3000u + registrations++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x4000u, "event group router");
        calls.emplace_back("group:" + std::to_string(key));
        return groups[key];
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        calls.emplace_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free:" + std::to_string(node));
    }
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void SubscribeAllBindings() {
    Runtime runtime;
    Check(pal4::CombatConsoleEventSubscriptions::Subscribe(
              kConsole, runtime) == 0x3016u &&
              runtime.key_calls == 23u && runtime.registrations == 23u,
          "all 23 callbacks register and final handle is returned");
    for (std::uint32_t index = 0u; index < kExpected.size(); ++index) {
        Check(runtime.words[kConsole + 68u + 8u * index] ==
                  0x3000u + index &&
                  runtime.words[kConsole + 72u + 8u * index] ==
                      0x2000u + index,
              "all 23 handle/key pairs use eight-byte stride");
    }
    Check(Position(runtime, "key:0") <
              Position(runtime, "register:0") &&
              Position(runtime, "register:15") <
              Position(runtime, "key:16") &&
              Position(runtime, "register:22") <
              Position(runtime, "write:4344"),
          "event key, registration and last slot storage order");
}

void UnsubscribeListAndMissingBranches() {
    Runtime runtime;
    const auto final_slot = kConsole + 68u + 8u * 22u;
    runtime.words[kConsole + 68u] = 0x3000u;
    runtime.words[kConsole + 72u] = 0x2000u;
    runtime.words[kConsole + 76u] = 0u;
    runtime.words[kConsole + 80u] = 0x9999u;
    runtime.words[final_slot] = 0x3016u;
    runtime.words[final_slot + 4u] = 0x2016u;
    runtime.groups[0x2000u] = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 2u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5004u] = 0x5200u;
    runtime.words[0x5100u] = 0x5200u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x7777u;
    runtime.words[0x5200u] = 0x5000u;
    runtime.words[0x5204u] = 0x5100u;
    runtime.words[0x520cu] = 0x3000u;
    pal4::CombatConsoleEventSubscriptions::Unsubscribe(kConsole, runtime);
    Check(runtime.words[0x5100u] == 0x5000u &&
              runtime.words[0x5004u] == 0x5100u &&
              runtime.words[0x6008u] == 1u,
          "matching second list node unlinked and group count decremented");
    Check(runtime.words[kConsole + 68u] == 0u &&
              runtime.words[kConsole + 72u] == 0u &&
              runtime.words[kConsole + 80u] == 0x9999u &&
              runtime.words[final_slot] == 0u &&
              runtime.words[final_slot + 4u] == 0u,
          "active pairs clear while zero-handle key stays untouched");
    Check(Position(runtime, "release:21000") <
              Position(runtime, "free:20992") &&
              Position(runtime, "free:20992") <
              Position(runtime, "write:24584"),
          "payload release precedes node free and count update");
}

void SubscribeRemovesOldHandlerFirst() {
    Runtime runtime;
    runtime.words[kConsole + 68u] = 0x4444u;
    runtime.words[kConsole + 72u] = 0x2000u;
    runtime.groups[0x2000u] = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 1u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5004u] = 0x5100u;
    runtime.words[0x5100u] = 0x5000u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x4444u;
    Check(pal4::CombatConsoleEventSubscriptions::Subscribe(
              kConsole, runtime) == 0x3016u &&
              runtime.words[0x6008u] == 0u &&
              Position(runtime, "free:20736") <
                  Position(runtime, "key:0"),
          "repeat registration removes prior callback before first new key");
}

}  // namespace

int main() {
    try {
        SubscribeAllBindings();
        UnsubscribeListAndMissingBranches();
        SubscribeRemovesOldHandlerFirst();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
