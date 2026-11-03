#include "pal4/combat_hint_window2.h"

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

constexpr std::uint32_t kWindow = 0x5000u;
constexpr std::uint32_t kRoot = 0x6000u;
constexpr std::uint32_t kEdit = 0x7000u;
constexpr std::uint32_t kManager = 0x9000u;
constexpr std::uint32_t kTimers = 0xa000u;

struct Runtime final : pal4::ICombatHintWindow2UiRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kWindow + 36u, kRoot}, {kWindow + 40u, kEdit},
        {kManager + 0x1a0u, kTimers}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::u32string shown_text;
    std::uint32_t parent = 0u;
    std::uint32_t position = 0u;
    std::uint32_t duration = 0u;
    std::uint32_t manager_calls = 0u;
    std::uint32_t default_calls = 0u;
    float x = 0.0f;
    float y = 0.0f;

    std::string ConvertToUtf8(std::string_view source) override {
        Check(source == "source", "conversion source");
        calls.emplace_back("convert");
        return std::string("A\xE4\xB8\xAD\xF0\x9F\x99\x82");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected word read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kWindow + 68u, "unexpected word write");
        words[address] = value;
        calls.emplace_back(value == 0xbf800000u ? "mark" : "clear");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kWindow + 64u, "unexpected byte write");
        bytes[address] = value;
        calls.emplace_back("flag");
    }
    void SetText(std::uint32_t edit, std::u32string_view text) override {
        Check(edit == kEdit && words.at(kWindow + 68u) == 0xbf800000u,
              "text target and active marker");
        shown_text.assign(text);
        calls.emplace_back("text");
    }
    std::uint32_t DefaultSheet() override {
        ++default_calls;
        calls.emplace_back("default-sheet");
        return 0x8100u;
    }
    void SetVisibility(std::uint32_t window, bool visible,
                       std::uint32_t new_parent) override {
        Check(window == kWindow && visible, "visible window");
        parent = new_parent;
        calls.emplace_back("visible");
    }
    void SetPosition(std::uint32_t root, std::uint32_t mode,
                     std::uint32_t value) override {
        Check(root == kRoot && mode == 0u, "position target and mode");
        position = value;
        calls.emplace_back("position");
    }
    void SetAlwaysOnTop(std::uint32_t root, bool enabled) override {
        Check(root == kRoot && enabled, "z-order target");
        calls.emplace_back("top");
    }
    float TextHeight(std::uint32_t) override { throw std::runtime_error("unexpected resize"); }
    float MaxLineWidth(std::uint32_t) override { throw std::runtime_error("unexpected resize"); }
    float ReadFloat(std::uint32_t) override { throw std::runtime_error("unexpected resize"); }
    void SetHeight(std::uint32_t, std::uint32_t, float) override {
        throw std::runtime_error("unexpected resize");
    }
    void SetWidth(std::uint32_t, std::uint32_t, float) override {
        throw std::runtime_error("unexpected resize");
    }
    float GetX(std::uint32_t root) override {
        Check(root == kRoot, "x target");
        return 100.0f;
    }
    float GetY(std::uint32_t root) override {
        Check(root == kRoot, "y target");
        return 50.0f;
    }
    float GetWidth(std::uint32_t root) override {
        Check(root == kRoot, "width target");
        return 80.0f;
    }
    float GetHeight(std::uint32_t root) override {
        Check(root == kRoot, "height target");
        return 20.0f;
    }
    void SetX(std::uint32_t root, float value) override {
        Check(root == kRoot, "set x target");
        x = value;
        calls.emplace_back("center-x");
    }
    void SetY(std::uint32_t root, float value) override {
        Check(root == kRoot, "set y target");
        y = value;
        calls.emplace_back("center-y");
    }
    std::uint32_t FrameManager() override {
        ++manager_calls;
        return kManager;
    }
    void RemoveQueued(std::uint32_t timers,
                      std::uint32_t clock_record) override {
        Check(timers == kTimers && clock_record == kWindow + 48u,
              "old timer identity");
        calls.emplace_back("remove-timer");
    }
    void QueueClose(std::uint32_t, std::uint32_t, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected first-hint timer API");
    }
    void QueueClose2(std::uint32_t timers, std::uint32_t clock_record,
                     std::uint32_t delay, std::uint32_t window,
                     std::uint32_t callback) override {
        Check(timers == kTimers && clock_record == kWindow + 48u &&
                  window == kWindow &&
                  callback == pal4::CombatHintWindow::kCleanupCallback,
              "second-hint timer identity");
        duration = delay;
        calls.emplace_back("queue-timer2");
    }
};

void ConvertedTextAndDefaultParent() {
    Runtime runtime;
    pal4::CombatHintWindow2::ShowText(
        kWindow, "source", 0x1234u, 200u, 0u, 7u, runtime);
    Check(runtime.shown_text == std::u32string{U'A', 0x4e2du, 0x1f642u} &&
              runtime.parent == 0x8100u && runtime.default_calls == 1u &&
              runtime.position == 0x1234u && runtime.duration == 200u &&
              runtime.manager_calls == 2u && runtime.x == 60.0f &&
              runtime.y == 40.0f && runtime.bytes.at(kWindow + 64u) == 7u &&
              runtime.words.at(kWindow + 68u) == 0u,
          "converted text and geometry");
    Check(runtime.calls == std::vector<std::string>{
              "convert", "mark", "flag", "text", "default-sheet",
              "visible", "position", "top", "center-x", "center-y",
              "remove-timer", "queue-timer2", "clear"},
          "second-hint show order");
}

void ExplicitParentAndEmptyText() {
    Runtime runtime;
    pal4::CombatHintWindow2::Show(
        kWindow, U"", 0x4321u, 0u, 0x8200u, 0u, runtime);
    Check(runtime.shown_text.empty() && runtime.parent == 0x8200u &&
              runtime.default_calls == 0u && runtime.position == 0x4321u &&
              runtime.duration == 0u && runtime.bytes.at(kWindow + 64u) == 0u,
          "explicit parent and zero duration");
}

}  // namespace

int main() {
    try {
        ConvertedTextAndDefaultParent();
        ExplicitParentAndEmptyText();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
