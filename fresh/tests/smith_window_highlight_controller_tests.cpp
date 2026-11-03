#include "pal4/smith_window_highlight_controller.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowHighlightRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x3000u;
    static constexpr std::uint32_t primary = 0x4000u;
    static constexpr std::uint32_t secondary = 0x5000u;
    bool primary_visible = true;
    bool secondary_visible = true;
    std::uint8_t flag = 0xffu;
    std::uint8_t primary_highlight = 0xffu;
    std::uint8_t secondary_highlight = 0xffu;
    std::vector<std::uint32_t> redraws;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 4u) return owner;
        if (address == owner + 0x58u) return primary;
        Check(address == owner + 0x5cu, "owner window offset");
        return secondary;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        if (address == object + 8u) {
            flag = value;
            calls.emplace_back("controller-flag");
        } else if (address == primary + 0x4b1u) {
            primary_highlight = value;
            calls.emplace_back("primary-highlight");
        } else {
            Check(address == secondary + 0x4b1u,
                  "highlight bit offset");
            secondary_highlight = value;
            calls.emplace_back("secondary-highlight");
        }
    }
    bool IsVisible(std::uint32_t window,
                   bool recursive) override {
        Check(!recursive, "visibility is non-recursive");
        if (window == primary) {
            calls.emplace_back("primary-visible");
            return primary_visible;
        }
        Check(window == secondary, "secondary visibility target");
        calls.emplace_back("secondary-visible");
        return secondary_visible;
    }
    void RequestRedraw(std::uint32_t window) override {
        redraws.push_back(window);
        calls.emplace_back("redraw");
    }
};

void CheckEnable() {
    Runtime visible;
    pal4::SmithWindowHighlightController::SetHighlighted(
        Runtime::object, 1u, visible);
    Check(visible.flag == 1u && visible.secondary_highlight == 1u &&
              visible.primary_highlight == 0xffu &&
              visible.redraws == std::vector<std::uint32_t>{Runtime::secondary} &&
              visible.calls == (std::vector<std::string>{
                  "controller-flag", "primary-visible",
                  "secondary-visible", "secondary-highlight", "redraw"}),
          "visible windows enable secondary highlight");

    Runtime hidden_primary;
    hidden_primary.primary_visible = false;
    pal4::SmithWindowHighlightController::SetHighlighted(
        Runtime::object, 1u, hidden_primary);
    Check(hidden_primary.flag == 0u &&
              hidden_primary.secondary_highlight == 1u &&
              hidden_primary.redraws ==
                  std::vector<std::uint32_t>{Runtime::secondary},
          "hidden primary clears controller flag but still highlights visible secondary");

    Runtime hidden_secondary;
    hidden_secondary.secondary_visible = false;
    pal4::SmithWindowHighlightController::SetHighlighted(
        Runtime::object, 1u, hidden_secondary);
    Check(hidden_secondary.flag == 1u &&
              hidden_secondary.secondary_highlight == 0xffu &&
              hidden_secondary.redraws.empty(),
          "hidden secondary skips highlight and redraw");
}

void CheckDisable() {
    Runtime runtime;
    Check(pal4::SmithWindowHighlightController::OnAutoClose(
              Runtime::object, runtime) == 1u &&
              runtime.flag == 0u &&
              runtime.secondary_highlight == 0u &&
              runtime.primary_highlight == 0u &&
              runtime.redraws == (std::vector<std::uint32_t>{
                  Runtime::secondary, Runtime::primary}) &&
              runtime.calls == (std::vector<std::string>{
                  "controller-flag", "primary-visible",
                  "secondary-highlight", "redraw",
                  "primary-highlight", "redraw"}),
          "auto-close clears both highlights and redraws both windows");
}

}  // namespace

int main() {
    try {
        CheckEnable();
        CheckDisable();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
