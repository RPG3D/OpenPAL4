#include "pal4/trade_window_input.h"

#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ITradeWindowInputRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x4004u, 0x7000u}, {0x7028u, 0x8000u},
        {0x7044u, 0x8100u}, {0x7048u, 0x8200u},
        {0x84c0u, 1u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{0x4008u, 0u}};
    std::map<std::uint32_t, bool> visible{
        {0x8100u, true}, {0x8200u, true}};
    std::vector<std::string> calls;
    std::int32_t rows = 3;
    std::uint32_t hovered = 0x9000u;
    std::int32_t hovered_row = 1;
    std::int32_t spinner = 5;
    std::int32_t set_spinner = 0;
    std::int32_t set_row = -1;
    std::uint32_t redrawn = 0u;
    std::uint32_t mouse_target = 0u;
    std::uint32_t closed_frame = 0u;
    std::uint32_t prompt_frame = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write-byte");
    }
    std::int32_t RowCount(std::uint32_t list) override {
        Check(list == 0x8000u, "row count list");
        calls.emplace_back("row-count");
        return rows;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x8000u, "hover list");
        calls.emplace_back("hover-item");
        return hovered;
    }
    void UpHoverItem(std::uint32_t list) override {
        Check(list == 0x8000u, "up list");
        calls.emplace_back("up");
    }
    void DownHoverItem(std::uint32_t list) override {
        Check(list == 0x8000u, "down list");
        calls.emplace_back("down");
    }
    void SetHoverRowIndex(std::uint32_t list,
                           std::int32_t row) override {
        Check(list == 0x8000u, "hover row list");
        set_row = row;
        calls.emplace_back("set-row");
    }
    bool IsVisible(std::uint32_t window, bool recurse) override {
        Check(!recurse, "visibility query flag");
        calls.emplace_back("visible");
        return visible.at(window);
    }
    void RequestRedraw(std::uint32_t window) override {
        redrawn = window;
        calls.emplace_back("redraw");
    }
    void HandleMouseEvent(std::uint32_t window) override {
        mouse_target = window;
        calls.emplace_back("mouse-event");
    }
    std::int32_t HoverRowOfItem(std::uint32_t list,
                                std::uint32_t item) override {
        Check(list == 0x8000u && item == 0x9000u,
              "hover grid reference");
        calls.emplace_back("hover-row");
        return hovered_row;
    }
    std::uint32_t GridItem(std::uint32_t list, std::int32_t row,
                            std::int32_t column) override {
        Check(list == 0x8000u && row == hovered_row && column == 1,
              "spinner grid reference");
        calls.emplace_back("grid-item");
        return 0x9100u;
    }
    std::int32_t SpinnerValue(std::uint32_t item) override {
        Check(item == 0x9100u, "spinner item");
        calls.emplace_back("spinner");
        return spinner;
    }
    void SetSpinnerValue(std::uint32_t item,
                         std::int32_t value) override {
        Check(item == 0x9100u, "set spinner item");
        set_spinner = value;
        calls.emplace_back("set-spinner");
    }
    void CloseTradeWindow(std::uint32_t frame) override {
        closed_frame = frame;
        calls.emplace_back("close");
    }
    void ShowTradePrompt(std::uint32_t frame) override {
        prompt_frame = frame;
        calls.emplace_back("prompt");
    }
};

Runtime WithKey(std::uint32_t key) {
    Runtime runtime;
    runtime.words[0x5010u] = key;
    return runtime;
}

void TestGlowAndStatus() {
    Runtime runtime;
    pal4::TradeWindowInput::SetItemGlow(0x4000u, true, runtime);
    Check(runtime.bytes.at(0x4008u) == 1u &&
              runtime.bytes.at(0x8200u + 0x4b1u) == 1u &&
              runtime.redrawn == 0x8200u &&
              runtime.calls == std::vector<std::string>{
                  "write-byte", "visible", "visible", "write-byte", "redraw"},
          "glow highlights right button when both arrows visible");
    runtime.calls.clear();
    Check(pal4::TradeWindowInput::OnStatusChanged(0x4000u, runtime) &&
              runtime.bytes.at(0x4008u) == 0u &&
              runtime.bytes.at(0x8200u + 0x4b1u) == 0u &&
              runtime.bytes.at(0x8100u + 0x4b1u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "write-byte", "visible", "write-byte", "redraw",
                  "write-byte", "redraw"},
          "status change clears both button highlights");

    Runtime hidden;
    hidden.visible[0x8100u] = false;
    pal4::TradeWindowInput::SetItemGlow(0x4000u, true, hidden);
    Check(hidden.bytes.at(0x4008u) == 0u &&
              hidden.bytes.at(0x8200u + 0x4b1u) == 1u,
          "hidden left arrow clears state even if right highlight is drawn");
    Runtime right_hidden;
    right_hidden.visible[0x8200u] = false;
    pal4::TradeWindowInput::SetItemGlow(0x4000u, true, right_hidden);
    Check(right_hidden.bytes.at(0x4008u) == 1u &&
              right_hidden.redrawn == 0u,
          "hidden right arrow skips highlight draw");
}

void TestKeyboardCommands() {
    auto escape = WithKey(1u);
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u, escape) &&
              escape.closed_frame == 0x7000u,
          "escape closes trade window");
    auto enter = WithKey(28u);
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u, enter) &&
              enter.prompt_frame == 0x7000u,
          "enter opens trade prompt");
    auto unknown = WithKey(999u);
    Check(!pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                    unknown) &&
              unknown.calls.empty(), "unknown key is not handled");
}

void TestKeyboardRows() {
    auto up = WithKey(200u);
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u, up) &&
              up.calls == std::vector<std::string>{
                  "row-count", "hover-item", "up"},
          "up key moves existing hover row");

    auto top = WithKey(200u);
    top.words[0x84c0u] = 0u;
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u, top) &&
              top.bytes.at(0x4008u) == 1u &&
              top.redrawn == 0x8200u,
          "up key on first row enters button glow");

    auto glowing_up = WithKey(200u);
    glowing_up.bytes[0x4008u] = 1u;
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                   glowing_up) &&
              glowing_up.calls.empty(),
          "up key does nothing while buttons glow");

    auto down = WithKey(208u);
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u, down) &&
              down.calls == std::vector<std::string>{
                  "row-count", "hover-item", "row-count", "down"},
          "down key advances before last row");
    auto last = WithKey(208u);
    last.words[0x84c0u] = 2u;
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u, last) &&
              last.calls.back() == "row-count" &&
              last.set_row == -1,
          "down key stays at last row");

    auto glowing_down = WithKey(208u);
    glowing_down.bytes[0x4008u] = 1u;
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                   glowing_down) &&
              glowing_down.bytes.at(0x4008u) == 0u &&
              glowing_down.set_row == 0,
          "down key exits glow and selects first row");

    for (const auto key : {200u, 208u}) {
        auto empty = WithKey(key);
        empty.rows = 0;
        Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                       empty) &&
                  empty.calls == std::vector<std::string>{"row-count"},
              "arrow key ignores empty list");
    }
    auto no_hover = WithKey(208u);
    no_hover.hovered = 0u;
    Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                   no_hover) &&
              no_hover.calls == std::vector<std::string>{
                  "row-count", "hover-item"},
          "down key ignores list without hovered item");
}

void TestKeyboardButtons() {
    for (const auto key : {203u, 205u}) {
        auto glowing = WithKey(key);
        glowing.bytes[0x4008u] = 1u;
        Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                       glowing) &&
                  glowing.redrawn == (key == 203u ? 0x8200u : 0x8100u) &&
                  glowing.mouse_target == (key == 203u ? 0x8100u : 0x8200u) &&
                  glowing.bytes.at(glowing.redrawn + 0x4b1u) == 0u &&
                  glowing.calls == std::vector<std::string>{
                      "write-byte", "redraw", "mouse-event"},
              "glowing lateral key clears opposite button and dispatches target");
        auto plain = WithKey(key);
        Check(pal4::TradeWindowInput::OnKeyboardInput(0x4000u, 0x5000u,
                                                       plain) &&
                  plain.calls.empty(),
              "lateral key without glow is handled without action");
    }
}

void TestMouse() {
    for (const auto key : {203u, 205u}) {
        auto runtime = WithKey(key);
        Check(pal4::TradeWindowInput::OnQuantityKeyInput(0x4000u, 0x5000u,
                                                   runtime) &&
                  runtime.set_spinner == (key == 203u ? 4 : 6) &&
                  runtime.redrawn == 0x8000u &&
                  runtime.calls == std::vector<std::string>{
                      "hover-item", "hover-row", "grid-item", "spinner",
                      "set-spinner", "redraw"},
              "mouse left/right changes spinner and redraws list");
        auto empty = WithKey(key);
        empty.hovered = 0u;
        Check(pal4::TradeWindowInput::OnQuantityKeyInput(0x4000u, 0x5000u,
                                                   empty) &&
                  empty.calls == std::vector<std::string>{"hover-item"},
              "mouse click with no hover is consumed");
    }
    auto glowing = WithKey(203u);
    glowing.bytes[0x4008u] = 1u;
    Check(pal4::TradeWindowInput::OnQuantityKeyInput(0x4000u, 0x5000u, glowing) &&
              glowing.mouse_target == 0x8100u &&
              glowing.calls == std::vector<std::string>{
                  "write-byte", "redraw", "mouse-event"},
          "mouse click while glowing uses keyboard button path");
    auto unknown = WithKey(999u);
    Check(!pal4::TradeWindowInput::OnQuantityKeyInput(0x4000u, 0x5000u, unknown),
          "unhandled mouse event forwards false from keyboard path");
}

}  // namespace

int main() {
    TestGlowAndStatus();
    TestKeyboardCommands();
    TestKeyboardRows();
    TestKeyboardButtons();
    TestMouse();
    return EXIT_SUCCESS;
}
