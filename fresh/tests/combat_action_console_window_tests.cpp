#include "pal4/combat_action_console_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;
constexpr std::uint32_t kName = 0x8b695cu;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActionConsoleWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::string fail_at;
    std::uint32_t mouse_count{};

    Runtime() {
        for (std::uint32_t index = 0u; index < 6u; ++index)
            words[kWindow + 0x150u + 4u * index] = 0x7000u + index;
    }
    void Call(const std::string& name) {
        calls.push_back(name);
        if (name == fail_at) throw std::runtime_error(name);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address >= kWindow &&
                  address + 4u <= kWindow + pal4::CombatActionConsoleWindow::kSize,
              "all constructor writes fit the 0x1e8-byte frame");
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void LoadLayout(std::uint32_t window, std::uint32_t name,
                    std::uint32_t first, std::uint32_t second) override {
        Check(window == kWindow && name == kName && first == 0u &&
                  second == 0u, "base layout constructor arguments");
        Call("layout");
    }
    void ConstructNavigation(std::uint32_t address) override {
        Check(address == kWindow + 0x28u, "navigation address");
        Call("navigation");
    }
    void ConstructMagicSelect(std::uint32_t address) override {
        Check(address == kWindow + 0x5cu, "magic selector address");
        Call("magic");
    }
    void ConstructPropertySelect(std::uint32_t address) override {
        Check(address == kWindow + 0xb4u, "property selector address");
        Call("property");
    }
    void ConstructStuntSelect(std::uint32_t address) override {
        Check(address == kWindow + 0x110u, "stunt selector address");
        Call("stunt");
    }
    void SetupUIElements(std::uint32_t address) override {
        Check(address == kWindow, "UI element setup owner");
        Call("setup");
    }
    std::uint32_t UpdateVisibility(std::uint32_t address,
                                   std::uint32_t first,
                                   std::uint32_t second) override {
        Check(address == kWindow && first == 0u && second == 0u,
              "initial visibility arguments");
        Call("visibility");
        return 0xa5u;
    }
    void SetupMouseHandlers(std::uint32_t navigation,
                            std::uint32_t button) override {
        Check(navigation == kWindow + 0x28u && mouse_count < 6u &&
                  button == 0x7000u + mouse_count,
              "mouse handlers use six button fields in order");
        ++mouse_count;
        Call("mouse");
    }
    void DestroyStuntSelect(std::uint32_t address) override {
        Check(address == kWindow + 0x110u, "stunt unwind address");
        calls.emplace_back("destroy-stunt");
    }
    void DestroyPropertySelect(std::uint32_t address) override {
        Check(address == kWindow + 0xb4u, "property unwind address");
        calls.emplace_back("destroy-property");
    }
    void DestroyMagicSelect(std::uint32_t address) override {
        Check(address == kWindow + 0x5cu, "magic unwind address");
        calls.emplace_back("destroy-magic");
    }
    void CleanupNavigation(std::uint32_t address) override {
        Check(address == kWindow + 0x28u, "navigation unwind address");
        calls.emplace_back("cleanup-navigation");
    }
    std::uint32_t DestructLayout(std::uint32_t address) override {
        Check(address == kWindow, "layout unwind address");
        calls.emplace_back("destruct-layout");
        return 0xb6u;
    }
    void DestructSelectorLayout(std::uint32_t address) override {
        calls.emplace_back("destruct-member:" + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void SetParent(std::uint32_t address, std::uint8_t active,
                   std::uint32_t parent) override {
        Check(address == kWindow && parent == 0x8888u && active <= 1u,
              "base parent arguments");
        calls.emplace_back("parent:" + std::to_string(active));
    }
};

struct EventRuntime final : pal4::IEventSubscriptionRuntime {
    Runtime& owner;
    std::uint32_t next{};

    explicit EventRuntime(Runtime& target) : owner(target) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return owner.words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        owner.WriteWord(address, value);
    }
    std::uint32_t EventKey(std::string_view name) override {
        constexpr std::array<std::string_view, 3> names{
            "re_start_ui_control", "re_close_ui_control", "combat_start"};
        Check(next < names.size() && name == names[next], "event name");
        owner.calls.emplace_back("key:" + std::to_string(next));
        return 0x6000u + next;
    }
    std::uint32_t EventRouter() override {
        owner.calls.emplace_back("router");
        return 0x4000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t window,
                           std::uint32_t callback) override {
        constexpr std::array<std::uint32_t, 3> callbacks{
            0x541570u, 0x5415a0u, 0x5415a0u};
        Check(next < callbacks.size() && router == 0x4000u && mode == 2u &&
                  key == 0x6000u + next && window == kWindow &&
                  callback == callbacks[next], "event subscription");
        owner.calls.emplace_back("register:" + std::to_string(next));
        return 0x9000u + next++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x4000u && key == 0x6000u,
              "cleanup event group");
        owner.calls.emplace_back("group");
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
};

void CheckSuccess() {
    Runtime runtime;
    Check(pal4::CombatActionConsoleWindow::Construct(
              kWindow, kName, runtime) == kWindow,
          "constructor returns the window object");
    Check(runtime.words.at(kWindow) == 0x845338u &&
              runtime.words.at(kWindow + 0x28u) == 0x845310u &&
              runtime.words.at(kWindow + 0x58u) == 0u &&
              runtime.words.at(kWindow + 0x1e4u) == 0u,
          "vtable and scalar fields");
    for (std::uint32_t offset = 0x40u; offset <= 0x54u; offset += 4u)
        Check(runtime.words.at(kWindow + offset) == 0u,
              "subscription slots initialized");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto record = kWindow + 0x168u + 20u * index;
        Check(runtime.words.at(record) == 0x7000u + index,
              "navigation node receives button");
        for (std::uint32_t field = 1u; field < 5u; ++field)
            Check(runtime.words.at(record + 4u * field) == 0u ||
                      runtime.words.at(record + 4u * field) >= kWindow,
                  "remaining navigation node fields initialized or linked");
    }
    constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 19> links{{
        {0x1b4u, 0x168u}, {0x1c0u, 0x168u}, {0x194u, 0x168u},
        {0x1a8u, 0x1b8u}, {0x1c8u, 0x17cu}, {0x16cu, 0x1b8u},
        {0x178u, 0x17cu}, {0x180u, 0x1b8u}, {0x188u, 0x1b8u},
        {0x1d0u, 0x17cu}, {0x1acu, 0x190u}, {0x1c4u, 0x1a4u},
        {0x170u, 0x190u}, {0x174u, 0x1a4u}, {0x1a0u, 0x1ccu},
        {0x19cu, 0x1a4u}, {0x184u, 0x1ccu}, {0x1d4u, 0x190u},
        {0x1d8u, 0x190u},
    }};
    for (const auto& [field, target] : links)
        Check(runtime.words.at(kWindow + field) == kWindow + target,
              "intrusive navigation pointers match IDA layout");
    Check(runtime.mouse_count == 6u &&
              runtime.calls.back() == "write:4568",
          "six mouse registrations precede final pointer links");
}

void CheckUnwind(const std::string& fail_at,
                 const std::vector<std::string>& expected) {
    Runtime runtime;
    runtime.fail_at = fail_at;
    bool caught = false;
    try {
        (void)pal4::CombatActionConsoleWindow::Construct(
            kWindow, kName, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.size() >= expected.size(),
          "constructor exception propagates");
    Check(std::vector<std::string>(runtime.calls.end() - expected.size(),
                                   runtime.calls.end()) == expected,
          "completed subobjects unwind in reverse order");
}

void CheckDestructor() {
    Runtime runtime;
    runtime.words[kWindow + 0x30u] = 0x7000u;
    runtime.words[kWindow + 0x34u] = 0x7004u;
    runtime.words[kWindow + 0x38u] = 0x7008u;
    Check(pal4::CombatActionConsoleWindow::Destruct(kWindow, runtime) ==
              0xb6u,
          "destructor forwards base result");
    Check(runtime.calls == (std::vector<std::string>{
              "destruct-member:4368", "destruct-member:4276",
              "destruct-member:4188", "free:28672", "write:4144",
              "write:4148", "write:4152", "destruct-layout"}),
          "selectors, navigation buffer and base destruct in order");
    Check(runtime.words[kWindow + 0x30u] == 0u &&
              runtime.words[kWindow + 0x34u] == 0u &&
              runtime.words[kWindow + 0x38u] == 0u,
          "navigation buffer slots cleared");
}

void CheckSetParent() {
    Runtime runtime;
    EventRuntime events(runtime);
    Check(pal4::CombatActionConsoleWindow::SetParentAndEvents(
              kWindow, 1u, 0x8888u, runtime, events) == 0xa5u &&
              events.next == 3u &&
              runtime.words[kWindow + 0x40u] == 0x9000u &&
              runtime.words[kWindow + 0x50u] == 0x9002u,
          "active parent registers three events and returns visibility result");
    Check(runtime.calls.front() == "parent:1" &&
              runtime.calls.back() == "visibility",
          "parent change precedes registration and visibility update");

    Runtime inactive;
    inactive.words[kWindow + 0x40u] = 0x9000u;
    inactive.words[kWindow + 0x44u] = 0x6000u;
    EventRuntime inactive_events(inactive);
    Check(pal4::CombatActionConsoleWindow::SetParentAndEvents(
              kWindow, 0u, 0x8888u, inactive, inactive_events) == 0xa5u &&
              inactive.words[kWindow + 0x40u] == 0u &&
              inactive.words[kWindow + 0x44u] == 0u &&
              inactive_events.next == 0u,
          "inactive parent removes event without registration");
    Check(inactive.calls.front() == "parent:0" &&
              inactive.calls.back() == "visibility",
          "inactive path still refreshes visibility last");
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckDestructor();
        CheckSetParent();
        CheckUnwind("layout", {"layout"});
        CheckUnwind("navigation", {"navigation", "destruct-layout"});
        CheckUnwind("magic", {"magic", "cleanup-navigation",
                               "destruct-layout"});
        CheckUnwind("property", {"property", "destroy-magic",
                                  "cleanup-navigation", "destruct-layout"});
        CheckUnwind("stunt", {"stunt", "destroy-property",
                               "destroy-magic", "cleanup-navigation",
                               "destruct-layout"});
        CheckUnwind("setup", {"setup", "destroy-stunt",
                               "destroy-property", "destroy-magic",
                               "cleanup-navigation", "destruct-layout"});
        CheckUnwind("mouse", {"mouse", "destroy-stunt",
                               "destroy-property", "destroy-magic",
                               "cleanup-navigation", "destruct-layout"});
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
