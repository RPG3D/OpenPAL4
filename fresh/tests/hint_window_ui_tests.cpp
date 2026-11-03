#include "pal4/hint_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IHintWindowUiRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::string conversion_input;
    std::string converted_text;
    std::u32string shown_text;
    std::uint32_t shown_edit = 0u;
    std::uint32_t shown_count = 0u;
    std::uint32_t visible_owner = 0u;
    std::uint32_t visible_parent = 0u;
    bool visible_state = false;
    pal4::HintPosition position{};
    std::uint32_t position_root = 0u;
    std::uint32_t position_metrics = ~0u;
    std::uint32_t queued_record = 0u;
    std::uint32_t queued_duration = 0u;
    std::uint32_t queued_owner = 0u;
    std::uint32_t queued_callback = 0u;
    std::uint32_t removed_parent = 0u;
    std::uint32_t removed_root = 0u;
    float x = 300.0f;
    float y = 240.0f;
    float width = 180.0f;
    float height = 80.0f;

    Runtime() {
        words[0x1024u] = 0x8000u;
        words[0x1028u] = 0x8200u;
        words[0x8044u] = 0x9000u;
        words[0x1044u] = 0u;
        bytes[0x1040u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == 0x1044u)
            calls.emplace_back(value == 0xbf800000u ?
                               "clock_negative" : "clock_zero");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("flag");
    }
    std::string ConvertToUtf8(std::string_view source) override {
        conversion_input = source;
        return converted_text;
    }
    void SetText(std::uint32_t edit,
                 std::u32string_view text) override {
        calls.emplace_back("text");
        shown_edit = edit;
        shown_text = text;
        ++shown_count;
    }
    std::uint32_t DefaultSheet() override {
        calls.emplace_back("default_sheet");
        return 0x9100u;
    }
    int SetVisibility(std::uint32_t window, bool visible,
                      std::uint32_t parent) override {
        calls.emplace_back(visible ? "show" : "hide");
        visible_owner = window;
        visible_parent = parent;
        visible_state = visible;
        return visible ? 17 : 23;
    }
    void SetPosition(std::uint32_t root, std::uint32_t metrics,
                     pal4::HintPosition value) override {
        calls.emplace_back("position");
        position_root = root;
        position_metrics = metrics;
        position = value;
    }
    void SetAlwaysOnTop(std::uint32_t root,
                         bool enabled) override {
        Check(root == 0x8000u && enabled, "root always on top");
        calls.emplace_back("top");
    }
    void PrepareLayout(std::uint32_t window) override {
        Check(window == 0x1000u, "layout owner");
        calls.emplace_back("layout");
    }
    void PrepareState(std::uint32_t window) override {
        Check(window == 0x1000u, "state owner");
        calls.emplace_back("state");
    }
    void RemoveQueued(std::uint32_t record) override {
        Check(record == 0x1030u, "remove queue key");
        calls.emplace_back("remove_queue");
    }
    void QueueClose(std::uint32_t record, std::uint32_t duration,
                    std::uint32_t window,
                    std::uint32_t callback) override {
        calls.emplace_back("queue");
        queued_record = record;
        queued_duration = duration;
        queued_owner = window;
        queued_callback = callback;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t root) override {
        calls.emplace_back("remove_child");
        removed_parent = parent;
        removed_root = root;
    }
    float GetX(std::uint32_t root) override {
        Check(root == 0x8000u, "get x root");
        return x;
    }
    float GetY(std::uint32_t root) override {
        Check(root == 0x8000u, "get y root");
        return y;
    }
    float GetWidth(std::uint32_t root) override {
        Check(root == 0x8000u, "get width root");
        return width;
    }
    float GetHeight(std::uint32_t root) override {
        Check(root == 0x8000u, "get height root");
        return height;
    }
    void SetX(std::uint32_t root, float value) override {
        Check(root == 0x8000u, "set x root");
        calls.emplace_back("set_x");
        x = value;
    }
    void SetY(std::uint32_t root, float value) override {
        Check(root == 0x8000u, "set y root");
        calls.emplace_back("set_y");
        y = value;
    }
};

void CheckShowAndConversion() {
    Runtime runtime;
    runtime.converted_text = "A\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
    pal4::HintWindow::ConvertAndShow(
        0x1000u, "source", {0.25f, 0.75f}, 500u, 0u, 1u, runtime);
    Check(runtime.conversion_input == "source" &&
              runtime.shown_edit == 0x8200u &&
              runtime.shown_text == U"A\u00e9\u4e2d\U0001f600" &&
              runtime.shown_count == 1u &&
              runtime.visible_owner == 0x1000u &&
              runtime.visible_state &&
              runtime.visible_parent == 0x9100u &&
              runtime.position_root == 0x8000u &&
              runtime.position_metrics == 0u &&
              runtime.position.x == 0.25f &&
              runtime.position.y == 0.75f &&
              runtime.queued_record == 0x1030u &&
              runtime.queued_duration == 500u &&
              runtime.queued_owner == 0x1000u &&
              runtime.queued_callback == 0x4abe30u &&
              runtime.words.at(0x1044u) == 0u &&
              runtime.bytes.at(0x1040u) == 1u,
          "converted text is displayed and the close timer is replaced");
    Check(runtime.calls == (std::vector<std::string>{
              "clock_negative", "flag", "text", "default_sheet",
              "show", "position", "top", "layout", "state",
              "remove_queue", "queue", "clock_zero"}),
          "hint display preserves the IDA operation order");

    Runtime explicit_parent;
    pal4::HintWindow::Show(0x1000u, U"message", {0.1f, 0.2f},
                           20u, 0x9200u, 0u, explicit_parent);
    Check(explicit_parent.visible_parent == 0x9200u &&
              explicit_parent.calls[3] == "show" &&
              explicit_parent.shown_text == U"message" &&
              explicit_parent.bytes.at(0x1040u) == 0u,
          "explicit parent bypasses default sheet lookup");

    runtime.converted_text = std::string("Hi\0ignored", 10u);
    pal4::HintWindow::ConvertAndShow(
        0x1000u, "again", {0.0f, 0.0f}, 1u, 0u, 0u, runtime);
    Check(runtime.shown_text == U"Hi" && runtime.shown_count == 2u,
          "embedded NUL terminates the converted message");
    runtime.converted_text = "\xE4\xB8";
    bool caught = false;
    try {
        pal4::HintWindow::ConvertAndShow(
            0x1000u, "truncated", {}, 1u, 0u, 0u, runtime);
    } catch (const std::length_error&) {
        caught = true;
    }
    Check(caught && runtime.shown_count == 2u,
          "truncated UTF-8 does not reach the display path");
}

void CheckCloseAndCenter() {
    Runtime runtime;
    Check(pal4::HintWindow::Close(0x1000u, runtime) == 23 &&
              runtime.removed_parent == 0x9000u &&
              runtime.removed_root == 0x8000u &&
              runtime.visible_parent == 0u &&
              !runtime.visible_state &&
              runtime.calls ==
                  (std::vector<std::string>{"remove_child", "hide"}),
          "close removes root before forwarding hide result");

    Runtime no_parent;
    no_parent.words[0x8044u] = 0u;
    Check(pal4::HintWindow::Close(0x1000u, no_parent) == 23 &&
              no_parent.calls == (std::vector<std::string>{"hide"}),
          "close without parent only forwards hide");

    Runtime center;
    pal4::HintWindow::Center(0x1000u, center);
    Check(center.x == 210.0f && center.y == 200.0f &&
              center.calls ==
                  (std::vector<std::string>{"set_x", "set_y"}),
          "center offsets the root by half its measured dimensions");
}

void CheckConditionalClose() {
    for (const auto [flag, clock, expected_close] :
         {std::tuple<std::uint8_t, std::uint32_t, bool>{0u, 0u, false},
          {1u, 0xbf800000u, false}, {1u, 0u, true},
          {1u, 0x3f800000u, true}, {1u, 0x7fc00000u, false}}) {
        Runtime runtime;
        runtime.bytes[0x1040u] = flag;
        runtime.words[0x1044u] = clock;
        Check(pal4::HintWindow::ConditionalClose(0x1000u, runtime) &&
                  (runtime.calls ==
                   (expected_close ?
                       std::vector<std::string>{"remove_child", "hide"} :
                       std::vector<std::string>{})),
              "z-order callback closes only for enabled nonnegative clock");
    }
}

}  // namespace

int main() {
    try {
        CheckShowAndConversion();
        CheckCloseAndCenter();
        CheckConditionalClose();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
