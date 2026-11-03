#include "pal4/timer_frame_layout.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::ITimerFrameLayoutRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> positions;
    std::vector<std::uint32_t> handlers;
    std::uint32_t height_window = 0u;
    std::uint32_t panel_window = 0u;
    std::uint32_t panel_height = 0u;
    std::uint32_t visible_window = 0u;
    bool visible = false;
    std::uint8_t modal = 0u;
    std::uint8_t parent_visible = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void SetVisible(std::uint32_t window, bool show) override {
        calls.emplace_back("visible");
        visible_window = window;
        visible = show;
    }
    void SetPosition(std::uint32_t window,
                     std::uint32_t x_bits,
                     std::uint32_t y_bits) override {
        calls.emplace_back("position");
        positions.emplace_back(window, x_bits, y_bits);
    }
    std::uint32_t HeightBits(std::uint32_t window) override {
        height_window = window;
        calls.emplace_back("height");
        return Bits(5.0f);
    }
    void SetHeight(std::uint32_t window,
                   std::uint32_t height_bits) override {
        calls.emplace_back("set-height");
        panel_window = window;
        panel_height = height_bits;
    }
    std::uint32_t SetupMouseHandler(std::uint32_t member,
                                    std::uint32_t window) override {
        Check(member == 0x1028u, "mouse handlers use +0x28 member");
        calls.emplace_back("mouse");
        handlers.push_back(window);
        return window + 1u;
    }
    void SetParent(std::uint32_t frame,
                   std::uint8_t show,
                   std::uint32_t parent) override {
        Check(frame == 0x1000u && parent == 0x7000u,
              "parent forwarding");
        calls.emplace_back("parent");
        parent_visible = show;
    }
    void ResetMember(std::uint32_t member) override {
        Check(member == 0x1028u, "visible path resets member");
        calls.emplace_back("reset");
    }
    void SetModalState(std::uint32_t window,
                       std::uint8_t show) override {
        Check(window == 0x3500u, "modal root window");
        calls.emplace_back("modal");
        modal = show;
    }
};

Runtime Seed() {
    Runtime runtime;
    runtime.words[0x1030u] = 0x2000u;
    runtime.words[0x1034u] = 0x1111u;
    runtime.words[0x1074u] = 1u;
    runtime.words[0x1058u] = 0x3000u;
    runtime.words[0x105cu] = 0x3100u;
    runtime.words[0x1060u] = 0x3200u;
    runtime.words[0x1064u] = 0x3300u;
    runtime.words[0x1068u] = 0x3400u;
    runtime.words[0x1048u] = Bits(10.0f);
    runtime.words[0x104cu] = Bits(20.0f);
    runtime.words[0x1050u] = Bits(30.0f);
    runtime.words[0x1054u] = Bits(40.0f);
    runtime.words[0x106cu] = Bits(100.0f);
    runtime.words[0x1024u] = 0x3500u;
    runtime.words[0x1070u] = 9u;
    return runtime;
}

void CheckExpanded() {
    auto runtime = Seed();
    Check(pal4::TimerFrameLayout::Apply(0x1000u, runtime) ==
              0x3201u && runtime.words[0x1034u] == 0x2000u &&
              runtime.visible_window == 0x3100u && runtime.visible &&
              runtime.height_window == 0x3200u,
          "expanded layout copies cursor, shows delegate and reads exit height");
    Check(runtime.positions ==
              std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                     std::uint32_t>>{
                  {0x3100u, Bits(10.0f), Bits(20.0f)},
                  {0x3200u, Bits(30.0f), Bits(40.0f)},
                  {0x3300u, Bits(30.0f), Bits(45.0f)}} &&
              runtime.panel_window == 0x3400u &&
              runtime.panel_height == Bits(100.0f) &&
              runtime.handlers == std::vector<std::uint32_t>{
                                      0x3000u, 0x3100u, 0x3200u},
          "expanded positions, original panel height and three mouse targets");
}

void CheckCollapsed() {
    auto runtime = Seed();
    runtime.words[0x1074u] = 0u;
    Check(pal4::TimerFrameLayout::Apply(0x1000u, runtime) ==
              0x3201u && !runtime.visible &&
              runtime.positions ==
                  std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                         std::uint32_t>>{
                      {0x3200u, Bits(10.0f), Bits(20.0f)},
                      {0x3300u, Bits(30.0f), Bits(40.0f)}} &&
              runtime.panel_height == Bits(95.0f) &&
              runtime.handlers ==
                  std::vector<std::uint32_t>{0x3000u, 0x3200u},
          "collapsed layout hides delegate and removes its mouse handler");
}

void CheckWindowVisibility() {
    auto shown = Seed();
    Check(pal4::TimerFrameLayout::SetWindowVisibility(
              0x1000u, 0x81u, 0x7000u, shown) == 0x3201u &&
              shown.parent_visible == 0x81u &&
              shown.modal == 0x81u &&
              shown.words[0x1070u] == 0u &&
              shown.calls[0] == "parent" &&
              shown.calls[1] == "reset" &&
              shown.calls[2] == "modal" &&
              shown.calls[3] == "visible",
          "show path resets member before modal and layout");

    auto hidden = Seed();
    hidden.words[0x1074u] = 0u;
    Check(pal4::TimerFrameLayout::SetWindowVisibility(
              0x1000u, 0u, 0x7000u, hidden) == 0x3201u &&
              hidden.words[0x1070u] == 9u &&
              hidden.calls[0] == "parent" &&
              hidden.calls[1] == "modal" &&
              hidden.calls[2] == "visible",
          "hide path preserves selection and skips member reset");
}

}  // namespace

int main() {
    CheckExpanded();
    CheckCollapsed();
    CheckWindowVisibility();
}
