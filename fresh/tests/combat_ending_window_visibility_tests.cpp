#include "pal4/combat_ending_window_visibility.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ArrayRuntime final : pal4::IEventHandlerArrayRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x4138u, 0x9000u}, {0x413cu, 0x9000u}, {0x4140u, 0x9010u}};
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t) override {
        throw std::runtime_error("unexpected array growth");
    }
    void Free(std::uint32_t) override {
        throw std::runtime_error("unexpected array free");
    }
    std::uint32_t InvokeVirtual(std::uint32_t object,
                                std::uint32_t slot) override {
        Check(object == kWindow + 40u && slot == 12u,
              "inserted ending handler callback");
        calls.emplace_back("invoke");
        return 0x77u;
    }
    std::uint8_t InvokePredicate(std::uint32_t, std::uint32_t,
                                 std::uint32_t) override {
        return 0u;
    }
};

struct Runtime final : pal4::ICombatEndingWindowVisibilityRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> hidden;

    Runtime() {
        for (std::uint32_t i = 0; i < 4u; ++i) {
            words[kWindow + 72u + 4u * i] = 0x5000u + i;
            words[kWindow + 116u + 76u * i] = 0x6000u + i;
            words[kWindow + 684u + 4u * i] = 0x7000u + i;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kWindow + 436u && value == 3u,
              "ending dialog mode field");
        words[address] = value;
        calls.emplace_back("mode");
    }
    void SetParent(std::uint32_t window, std::uint8_t visible,
                   std::uint32_t parent) override {
        Check(window == kWindow && parent == 0xa000u && visible <= 1u,
              "base visibility arguments");
        calls.emplace_back("parent:" + std::to_string(visible));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(!visible, "all subwindows are hidden");
        hidden.push_back(window);
        calls.emplace_back("hide:" + std::to_string(window));
    }
    void CleanupHint(std::uint32_t hint) override {
        calls.emplace_back("hint:" + std::to_string(hint));
    }
    void PlayStoredSound(std::uint32_t handle_address) override {
        calls.emplace_back("sound:" + std::to_string(handle_address));
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame");
        return 0x4000u;
    }
    std::uint32_t ResetListFlags(std::uint32_t list) override {
        Check(list == kWindow + 44u, "list reset offset");
        calls.emplace_back("reset");
        return 0x88u;
    }
};

void CheckCommon(const Runtime& runtime, std::uint8_t visible) {
    Check(runtime.calls.front() ==
              "parent:" + std::to_string(visible),
          "base parent first");
    const std::array<std::uint32_t, 8> expected{
        0x5003u, 0x5002u, 0x5001u, 0x5000u,
        0x6000u, 0x6001u, 0x6002u, 0x6003u};
    Check(runtime.hidden.size() == expected.size(), "eight hidden windows");
    for (std::uint32_t i = 0; i < expected.size(); ++i)
        Check(runtime.hidden[i] == expected[i], "hidden window order");
    for (std::uint32_t i = 0; i < 4u; ++i) {
        const auto base = 5u + 3u * i;
        Check(runtime.calls[base] ==
                  "hide:" + std::to_string(0x6000u + i) &&
                  runtime.calls[base + 1u] ==
                      "hint:" + std::to_string(0x7000u + i) &&
                  runtime.calls[base + 2u] ==
                      "sound:" + std::to_string(kWindow + 700u + 4u * i),
              "each hint hidden, cleaned and sounded in order");
    }
    Check(runtime.calls[17] == "mode" &&
              runtime.calls[18] == "frame",
          "mode changes before handler array access");
}

void ShowAndHide() {
    Runtime shown;
    ArrayRuntime array;
    Check(pal4::CombatEndingWindowVisibility::Update(
              kWindow, 1u, 0xa000u, shown, array) == 0x77u,
          "show returns inserted handler callback");
    CheckCommon(shown, 1u);
    Check(shown.calls.size() == 19u &&
              array.words[0x9000u] == kWindow + 40u &&
              array.words[0x413cu] == 0x9004u &&
              array.calls.back() == "invoke",
          "show inserts handler without reset");

    Runtime hidden;
    Check(pal4::CombatEndingWindowVisibility::Update(
              kWindow, 0u, 0xa000u, hidden, array) == 0x88u,
          "hide forwards reset-list return");
    CheckCommon(hidden, 0u);
    Check(hidden.calls.back() == "reset" &&
              array.words[0x413cu] == 0x9000u,
          "hide removes handler before list reset");
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
