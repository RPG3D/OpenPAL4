#include "pal4/combat_ending_window_input.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;
constexpr std::uint32_t kEvent = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingWindowInputRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kWindow + 436u && value == 3u,
              "finished mode write");
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x3000u;
    }
    std::uint32_t EndCombatSequence(std::uint32_t system,
                                    std::uint32_t prior_mode) override {
        Check(system == 0x3000u && prior_mode ==
                  words[kWindow + 436u],
              "old mode forwarded before reset");
        calls.emplace_back("end:" + std::to_string(prior_mode));
        return 0x77u;
    }
    void ShowResult(std::uint32_t window) override {
        Check(window == kWindow, "show result owner");
        calls.emplace_back("show");
    }
    void HideResult(std::uint32_t window) override {
        Check(window == kWindow, "hide result owner");
        calls.emplace_back("hide");
    }
};

void DirectFinish() {
    Runtime runtime;
    runtime.words[kWindow + 436u] = 2u;
    Check(pal4::CombatEndingWindowInput::FinishCombat(kWindow, runtime) ==
              0x77u,
          "finish forwards end-sequence result");
    Check(runtime.calls[0] == "system" &&
              runtime.calls[1] == "read:4532" &&
              runtime.calls[2] == "end:2" &&
              runtime.calls[3] == "write" &&
              runtime.words[kWindow + 436u] == 3u,
          "combat end happens before mode reset");
}

void KeyBranches() {
    Runtime stopped;
    stopped.words[kWindow + 436u] = 3u;
    stopped.words[kEvent + 16u] = 1u;
    Check(pal4::CombatEndingWindowInput::HandleInput(
              kWindow + 40u, kEvent, stopped) == 0u &&
              stopped.calls.size() == 1u,
          "mode 3 ignores input before reading key");

    Runtime other;
    other.words[kWindow + 436u] = 0u;
    other.words[kEvent + 16u] = 7u;
    Check(pal4::CombatEndingWindowInput::HandleInput(
              kWindow + 40u, kEvent, other) == 0u &&
              other.calls.size() == 2u,
          "unhandled key returns zero");

    Runtime end;
    end.words[kWindow + 436u] = 1u;
    end.words[kEvent + 16u] = 1u;
    Check(pal4::CombatEndingWindowInput::HandleInput(
              kWindow + 40u, kEvent, end) == 1u &&
              end.words[kWindow + 436u] == 3u &&
              end.calls[end.calls.size() - 2] == "end:1",
          "key 1 finishes combat");

    for (std::uint32_t state = 0u; state < 3u; ++state) {
        Runtime runtime;
        runtime.words[kWindow + 436u] = 0u;
        runtime.words[kWindow + 716u] = state;
        runtime.words[kEvent + 16u] = 28u;
        Check(pal4::CombatEndingWindowInput::HandleInput(
                  kWindow + 40u, kEvent, runtime) == 1u,
              "key 28 handled");
        if (state == 0u)
            Check(runtime.calls.back() == "show", "key 28 opens result");
        else if (state == 1u)
            Check(runtime.calls.back() == "hide", "key 28 closes result");
        else
            Check(runtime.calls.back() == "write" &&
                      runtime.words[kWindow + 436u] == 3u,
                  "key 28 ends after other result state");
    }
}

void MouseBranches() {
    Runtime ignored;
    ignored.words[kEvent + 28u] = 1u;
    Check(pal4::CombatEndingWindowInput::HandleMouseDown(
              kWindow, kEvent, ignored) == 1u &&
              ignored.calls.size() == 1u,
          "nonzero mouse field returns handled without action");

    for (std::uint32_t state = 0u; state < 3u; ++state) {
        Runtime runtime;
        runtime.words[kWindow + 436u] = 2u;
        runtime.words[kWindow + 716u] = state;
        Check(pal4::CombatEndingWindowInput::HandleMouseDown(
                  kWindow, kEvent, runtime) == 1u,
              "mouse-down always handled");
        if (state == 0u)
            Check(runtime.calls.back() == "show", "mouse opens result");
        else if (state == 1u)
            Check(runtime.calls.back() == "hide", "mouse closes result");
        else
            Check(runtime.calls.back() == "write" &&
                      runtime.words[kWindow + 436u] == 3u,
                  "mouse ends after other result state");
    }
}

}  // namespace

int main() {
    try {
        DirectFinish();
        KeyBranches();
        MouseBranches();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
