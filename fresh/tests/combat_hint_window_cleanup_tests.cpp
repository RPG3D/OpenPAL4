#include "pal4/combat_hint_window.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kWindow = 0x5000u;
constexpr std::uint32_t kRoot = 0x6000u;
constexpr std::uint32_t kParent = 0x7000u;
constexpr std::uint32_t kManager = 0x9000u;
constexpr std::uint32_t kTimers = 0xa000u;

struct Runtime final : pal4::ICombatHintWindowCleanupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kWindow + 36u, kRoot}, {kRoot + 68u, kParent},
        {kWindow + 68u, 0u}, {kManager + 0x1a0u, kTimers}};
    std::uint8_t flag = 1u;
    std::vector<std::string> calls;
    std::uint32_t result = 0x12345678u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected word read");
        if (address == kRoot + 68u) calls.emplace_back("read-parent");
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kWindow + 64u, "flag address");
        calls.emplace_back("read-flag");
        return flag;
    }
    void RemoveChild(std::uint32_t parent, std::uint32_t child) override {
        Check(parent == kParent && child == kRoot, "remove root from parent");
        calls.emplace_back("remove-child");
    }
    void SetVisibility(std::uint32_t window, bool visible,
                       std::uint32_t parent) override {
        Check(window == kWindow && !visible && parent == 0u,
              "hide hint window");
        calls.emplace_back("hide");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return kManager;
    }
    std::uint32_t RemoveTimer(std::uint32_t timers,
                              std::uint32_t clock_record) override {
        Check(timers == kTimers && clock_record == kWindow + 48u,
              "cleanup timer identity");
        calls.emplace_back("remove-timer");
        return result;
    }
};

void CleanupWithAndWithoutParent() {
    Runtime runtime;
    Check(pal4::CombatHintWindow::Cleanup(kWindow, runtime) == runtime.result,
          "cleanup forwards timer removal result");
    Check(runtime.calls == std::vector<std::string>{
              "read-parent", "remove-child", "hide", "manager",
              "remove-timer"},
          "parent removal precedes hide and timer removal");

    Runtime detached;
    detached.words[kRoot + 68u] = 0u;
    (void)pal4::CombatHintWindow::Cleanup(kWindow, detached);
    Check(detached.calls == std::vector<std::string>{
              "read-parent", "hide", "manager", "remove-timer"},
          "detached root needs no parent removal");
}

void ZOrderFlagAndFloatGate() {
    Runtime disabled;
    disabled.flag = 0u;
    Check(pal4::CombatHintWindow::OnZOrderChanged(kWindow, disabled) &&
              disabled.calls == std::vector<std::string>{"read-flag"},
          "zero flag skips float and cleanup");

    for (const float value : {-1.0f, -0.01f,
                              std::numeric_limits<float>::quiet_NaN()}) {
        Runtime inactive;
        inactive.words[kWindow + 68u] = std::bit_cast<std::uint32_t>(value);
        Check(pal4::CombatHintWindow::OnZOrderChanged(kWindow, inactive) &&
                  inactive.calls == std::vector<std::string>{"read-flag"},
              "negative or unordered float skips cleanup");
    }

    for (const float value : {0.0f, 1.0f}) {
        Runtime active;
        active.words[kWindow + 68u] = std::bit_cast<std::uint32_t>(value);
        Check(pal4::CombatHintWindow::OnZOrderChanged(kWindow, active) &&
                  active.calls == std::vector<std::string>{
                      "read-flag", "read-parent", "remove-child", "hide",
                      "manager", "remove-timer"},
              "nonnegative float closes flagged hint");
    }
}

}  // namespace

int main() {
    try {
        CleanupWithAndWithoutParent();
        ZOrderFlagAndFloatGate();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
