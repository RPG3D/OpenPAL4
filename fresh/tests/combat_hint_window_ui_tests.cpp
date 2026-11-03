#include "pal4/combat_hint_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kWindow = 0x5000u;
constexpr std::uint32_t kRoot = 0x6000u;
constexpr std::uint32_t kEdit = 0x7000u;
constexpr std::uint32_t kManager = 0x9000u;
constexpr std::uint32_t kTimers = 0xa000u;

struct Runtime final : pal4::ICombatHintWindowUiRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kWindow + 36u, kRoot}, {kWindow + 40u, kEdit},
        {kWindow + 44u, 0x7100u},
        {kManager + 0x1a0u, kTimers}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::u32string shown_text;
    std::uint32_t visibility_parent = 0u;
    std::uint32_t position = 0u;
    std::uint32_t duration = 0u;
    std::uint32_t manager_calls = 0u;
    std::uint32_t default_calls = 0u;
    float x = 0.0f;
    float y = 0.0f;
    float edit_height = 0.0f;
    float frame_height = 0.0f;
    float edit_width = 0.0f;
    float frame_width = 0.0f;

    std::string ConvertToUtf8(std::string_view source) override {
        Check(source == "source", "source forwarded to text converter");
        calls.emplace_back("convert");
        return std::string("A\xE4\xB8\xAD\xF0\x9F\x99\x82");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected hint UI word read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kWindow + 68u)
            calls.emplace_back(value == 0xbf800000u ? "mark" : "clear");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetText(std::uint32_t edit,
                 std::u32string_view text) override {
        Check(edit == kEdit && words.at(kWindow + 68u) == 0xbf800000u,
              "text set while hint is marked active");
        shown_text.assign(text);
        calls.emplace_back("text");
    }
    std::uint32_t DefaultSheet() override {
        ++default_calls;
        calls.emplace_back("default-sheet");
        return 0x8100u;
    }
    void SetVisibility(std::uint32_t window, bool visible,
                       std::uint32_t parent) override {
        Check(window == kWindow && visible, "hint becomes visible");
        visibility_parent = parent;
        calls.emplace_back("visible");
    }
    void SetPosition(std::uint32_t root, std::uint32_t mode,
                     std::uint32_t new_position) override {
        Check(root == kRoot && mode == 0u, "position metrics mode");
        position = new_position;
        calls.emplace_back("position");
    }
    void SetAlwaysOnTop(std::uint32_t root,
                        bool enabled) override {
        Check(root == kRoot && enabled, "hint z-order");
        calls.emplace_back("top");
    }
    float TextHeight(std::uint32_t edit) override {
        Check(edit == kEdit, "text height target");
        calls.emplace_back("text-height");
        return 19.0f;
    }
    float MaxLineWidth(std::uint32_t edit) override {
        Check(edit == kEdit, "max line width target");
        calls.emplace_back("line-width");
        return 33.0f;
    }
    float ReadFloat(std::uint32_t address) override {
        if (address == kEdit + 272u) return 10.0f;
        if (address == 0x7100u + 272u) return 50.0f;
        if (address == kEdit + 268u) return 25.0f;
        if (address == 0x7100u + 268u) return 100.0f;
        Check(false, "unexpected dimension read");
        return 0.0f;
    }
    void SetHeight(std::uint32_t window, std::uint32_t mode,
                   float value) override {
        Check(mode == 1u, "pixel height mode");
        if (window == kEdit) {
            edit_height = value;
            calls.emplace_back("edit-height");
        } else {
            Check(window == 0x7100u, "frame height target");
            frame_height = value;
            calls.emplace_back("frame-height");
        }
    }
    void SetWidth(std::uint32_t window, std::uint32_t mode,
                  float value) override {
        Check(mode == 1u, "pixel width mode");
        if (window == kEdit) {
            edit_width = value;
            calls.emplace_back("edit-width");
        } else {
            Check(window == 0x7100u, "frame width target");
            frame_width = value;
            calls.emplace_back("frame-width");
        }
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
              "existing timer removed by clock id");
        calls.emplace_back("remove-timer");
    }
    void QueueClose(std::uint32_t timers, std::uint32_t clock_record,
                    std::uint32_t delay, std::uint32_t window,
                    std::uint32_t callback) override {
        Check(timers == kTimers && clock_record == kWindow + 48u &&
                  window == kWindow &&
                  callback == pal4::CombatHintWindow::kCleanupCallback,
              "close timer identity");
        duration = delay;
        calls.emplace_back("queue-timer");
    }
};

void ConvertedTextAndDefaultParent() {
    Runtime runtime;
    pal4::CombatHintWindow::ShowText(
        kWindow, "source", 0x1234u, 200u, 0u, 7u, runtime);
    Check(runtime.shown_text == std::u32string{U'A', 0x4e2du, 0x1f642u} &&
              runtime.visibility_parent == 0x8100u &&
              runtime.default_calls == 1u &&
              runtime.position == 0x1234u &&
              runtime.duration == 200u &&
              runtime.manager_calls == 2u &&
              runtime.x == 60.0f && runtime.y == 40.0f &&
              runtime.edit_height == 20.0f &&
              runtime.frame_height == 60.0f &&
              runtime.edit_width == 40.0f &&
              runtime.frame_width == 115.0f &&
              runtime.bytes.at(kWindow + 64u) == 7u &&
              runtime.words.at(kWindow + 68u) == 0u,
          "UTF-8 text, default sheet, centering and close timer");
    Check(runtime.calls == std::vector<std::string>{
              "convert", "mark", "text", "default-sheet", "visible",
              "position", "top", "text-height", "edit-height",
              "frame-height", "line-width", "edit-width",
              "frame-width", "center-x", "center-y",
              "remove-timer", "queue-timer", "clear"},
          "show path preserves visible and timer order");
}

void ExplicitParentAndEmptyText() {
    Runtime runtime;
    pal4::CombatHintWindow::Show(
        kWindow, U"", 0x4321u, 0u, 0x8200u, 0u, runtime);
    Check(runtime.shown_text.empty() &&
              runtime.visibility_parent == 0x8200u &&
              runtime.default_calls == 0u &&
              runtime.duration == 0u &&
              runtime.bytes.at(kWindow + 64u) == 0u,
          "explicit parent and zero-duration path");
    Check(pal4::CombatHintWindow::SetFlag(
              kWindow, 0xa5u, runtime) == 0xa5u &&
              runtime.bytes.at(kWindow + 64u) == 0xa5u,
          "flag setter writes one byte and returns the input");
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
