#include "pal4/combat_action_console_window_events.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;
constexpr std::array<std::string_view, 3> kNames{
    "re_start_ui_control", "re_close_ui_control", "combat_start"};
constexpr std::array<std::uint32_t, 3> kCallbacks{
    0x541570u, 0x5415a0u, 0x5415a0u};

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IEventSubscriptionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> groups;
    std::vector<std::string> calls;
    std::uint32_t next_key{};
    std::uint32_t next_register{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t EventKey(std::string_view name) override {
        Check(next_key < kNames.size() && name == kNames[next_key],
              "event name and order");
        calls.emplace_back("key:" + std::to_string(next_key));
        return 0x2000u + next_key++;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x4000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(next_register < kNames.size() && router == 0x4000u &&
                  mode == 2u && key == 0x2000u + next_register &&
                  owner == kWindow &&
                  callback == kCallbacks[next_register],
              "router, mode, key, owner and callback");
        calls.emplace_back("register:" + std::to_string(next_register));
        return 0x3000u + next_register++;
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

std::size_t Position(const Runtime& runtime, const std::string& call) {
    for (std::size_t index = 0u; index < runtime.calls.size(); ++index)
        if (runtime.calls[index] == call) return index;
    throw std::runtime_error("expected call missing: " + call);
}

void CheckInitialize() {
    Runtime runtime;
    Check(pal4::CombatActionConsoleWindowEvents::Initialize(
              kWindow, runtime) == 0x3002u &&
              runtime.next_key == 3u && runtime.next_register == 3u,
          "three subscriptions and last handle result");
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto slot = kWindow + 0x40u + 8u * index;
        Check(runtime.words[slot] == 0x3000u + index &&
                  runtime.words[slot + 4u] == 0x2000u + index,
              "handle and key occupy adjacent fields");
    }
    Check(Position(runtime, "key:0") <
              Position(runtime, "register:0") &&
              Position(runtime, "register:2") <
              Position(runtime, "write:4180") &&
              Position(runtime, "write:4180") <
              Position(runtime, "write:4176"),
          "third subscription writes key before handle");
}

void CheckCleanup() {
    Runtime runtime;
    runtime.words[kWindow + 0x40u] = 0x3000u;
    runtime.words[kWindow + 0x44u] = 0x2000u;
    runtime.words[kWindow + 0x48u] = 0u;
    runtime.words[kWindow + 0x4cu] = 0x9999u;
    runtime.words[kWindow + 0x50u] = 0x3002u;
    runtime.words[kWindow + 0x54u] = 0x2002u;
    runtime.groups[0x2000u] = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 1u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5004u] = 0x5100u;
    runtime.words[0x5100u] = 0x5000u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x3000u;
    Check(pal4::CombatActionConsoleWindowEvents::Cleanup(
              kWindow, runtime) == 0u,
          "cleanup returns loop count zero");
    Check(runtime.words[0x5000u] == 0x5000u &&
              runtime.words[0x5004u] == 0x5000u &&
              runtime.words[0x6008u] == 0u,
          "matching node is unlinked and count decremented");
    Check(runtime.words[kWindow + 0x40u] == 0u &&
              runtime.words[kWindow + 0x44u] == 0u &&
              runtime.words[kWindow + 0x4cu] == 0x9999u &&
              runtime.words[kWindow + 0x50u] == 0u &&
              runtime.words[kWindow + 0x54u] == 0u,
          "active pairs are cleared and inactive key retained");
    Check(Position(runtime, "release:20744") <
              Position(runtime, "free:20736") &&
              Position(runtime, "free:20736") <
              Position(runtime, "write:24584"),
          "owned payload releases before node and count");
}

void CheckReinitialize() {
    Runtime runtime;
    runtime.words[kWindow + 0x40u] = 0x4444u;
    runtime.words[kWindow + 0x44u] = 0x2000u;
    Check(pal4::CombatActionConsoleWindowEvents::Initialize(
              kWindow, runtime) == 0x3002u &&
              Position(runtime, "group:8192") <
                  Position(runtime, "key:0") &&
              runtime.words[kWindow + 0x40u] == 0x3000u,
          "old event is cleaned before the first new registration");
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        CheckCleanup();
        CheckReinitialize();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
