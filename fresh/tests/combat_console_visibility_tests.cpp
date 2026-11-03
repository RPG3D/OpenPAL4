#include "pal4/combat_console.h"

#include <algorithm>
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

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kParent = 0x6000u;
constexpr std::uint32_t kManager = 0x9000u;
constexpr std::uint32_t kTimers = 0xa000u;

struct Runtime final : pal4::ICombatConsoleVisibilityRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kConsole + 216u, 0x8000u}, {kConsole + 260u, 0x8100u},
        {kManager + 416u, kTimers}};
    std::vector<std::string> calls;
    std::uint32_t keys = 0u;
    std::uint32_t registrations = 0u;
    std::uint32_t group_lookups = 0u;
    std::uint32_t manager_calls = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address == kConsole + 308u)
            calls.emplace_back("reset-first");
        if (address == kConsole + 312u)
            calls.emplace_back("reset-second");
    }
    std::uint32_t EventKey(std::string_view name) override {
        Check(!name.empty(), "event key name");
        calls.emplace_back("key:" + std::to_string(keys));
        return 0x2000u + keys++;
    }
    std::uint32_t EventRouter() override { return 0x4000u; }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x4000u && mode >= 1u && mode <= 3u &&
                  key == 0x2000u + registrations && owner == kConsole &&
                  callback != 0u,
              "event registration arguments");
        calls.emplace_back("register:" + std::to_string(registrations));
        return 0x3000u + registrations++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x4000u && key >= 0x2000u && key < 0x2013u,
              "active event group lookup");
        ++group_lookups;
        calls.emplace_back("group");
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {
        throw std::runtime_error("missing group cannot release node");
    }
    void FreeNode(std::uint32_t) override {
        throw std::runtime_error("missing group cannot free node");
    }
    void SetParent(std::uint32_t console, bool visible,
                   std::uint32_t parent) override {
        Check(console == kConsole && parent == kParent,
              "visibility parent forwarding");
        calls.emplace_back(visible ? "show-parent" : "hide-parent");
    }
    void SetChildVisible(std::uint32_t child, bool visible) override {
        Check(!visible && (child == 0x8000u || child == 0x8100u),
              "two child controls hidden");
        calls.emplace_back(child == 0x8000u ? "hide-first-child"
                                            : "hide-second-child");
    }
    std::uint32_t FrameManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return kManager;
    }
    std::uint32_t AddHandler(std::uint32_t handlers,
                             std::uint32_t handler) override {
        Check(handlers == kManager + 308u && handler == kConsole + 40u,
              "add frame event handler");
        calls.emplace_back("add-handler");
        return 0xbbbb0001u;
    }
    void RemoveHandler(std::uint32_t handlers,
                       std::uint32_t handler) override {
        Check(handlers == kManager + 308u && handler == kConsole + 40u,
              "remove frame event handler");
        calls.emplace_back("remove-handler");
    }
    std::uint32_t RemoveTimer(std::uint32_t timers,
                              std::uint32_t name) override {
        Check(timers == kTimers && name == kConsole + 276u,
              "reset-command timer identity");
        calls.emplace_back("remove-timer");
        return 0xeeee0002u;
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void ShowAndHide() {
    Runtime runtime;
    Check(pal4::CombatConsoleWindow::SetVisible(
              kConsole, true, kParent, runtime) == 0xbbbb0001u &&
              runtime.keys == 19u && runtime.registrations == 19u &&
              runtime.manager_calls == 1u &&
              runtime.words[kConsole + 308u] == 0u &&
              runtime.words[kConsole + 312u] == 0xc0200000u,
          "show registers events and resets state");
    Check(Position(runtime, "show-parent") <
              Position(runtime, "hide-first-child") &&
              Position(runtime, "hide-first-child") <
                  Position(runtime, "key:0") &&
              Position(runtime, "register:18") <
                  Position(runtime, "hide-second-child") &&
              Position(runtime, "hide-second-child") <
                  Position(runtime, "add-handler") &&
              Position(runtime, "add-handler") <
                  Position(runtime, "reset-first"),
          "show phase ordering");

    runtime.calls.clear();
    runtime.manager_calls = 0u;
    Check(pal4::CombatConsoleWindow::SetVisible(
              kConsole, false, kParent, runtime) == 0xeeee0002u &&
              runtime.group_lookups == 19u &&
              runtime.manager_calls == 2u &&
              runtime.words[kConsole + 308u] == 0u &&
              runtime.words[kConsole + 312u] == 0xc0200000u,
          "hide cleans events, removes handler and timer");
    for (std::uint32_t index = 0u; index < 19u; ++index) {
        Check(runtime.words[kConsole + 60u + 8u * index] == 0u &&
                  runtime.words[kConsole + 64u + 8u * index] == 0u,
              "all active subscription slots cleared");
    }
    Check(Position(runtime, "hide-parent") < Position(runtime, "group") &&
              Position(runtime, "group") <
                  Position(runtime, "remove-handler") &&
              Position(runtime, "remove-handler") <
                  Position(runtime, "remove-timer") &&
              Position(runtime, "remove-timer") <
                  Position(runtime, "reset-first"),
          "hide phase ordering");
}

}  // namespace

int main() {
    try {
        ShowAndHide();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
