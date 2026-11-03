#include "pal4/magic_window_action_buttons.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowActionButtonRuntimeStub {
    std::vector<std::string> calls;
    void SetVisible(std::uint32_t button, bool visible) override {
        Check(!visible, "reset hides button");
        calls.push_back("hide:" + std::to_string(button));
    }
    void SetEnabled(std::uint32_t button, bool enabled) override {
        Check(enabled, "reset enables button");
        calls.push_back("enable:" + std::to_string(button));
    }
    void SetKey(std::uint32_t button, std::uint32_t key) override {
        Check(key == 0u, "reset clears button key");
        calls.push_back("key:" + std::to_string(button));
    }
};

void TestResetAllSixButtons() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t index = 0; index < 6u; ++index)
        panel.At(0xf0u + 4u * index) = 0x2000u + index;
    Runtime runtime;
    pal4::MagicWindowActionButtons::Reset(panel, runtime);
    Check(runtime.calls.size() == 18u, "six buttons receive three operations");
    for (std::uint32_t index = 0; index < 6u; ++index) {
        const auto button = std::to_string(0x2000u + index);
        Check(runtime.calls[3u * index] == "hide:" + button &&
                  runtime.calls[3u * index + 1u] == "enable:" + button &&
                  runtime.calls[3u * index + 2u] == "key:" + button,
              "reset preserves per-button order and panel offsets");
    }
}

}  // namespace

int main() {
    TestResetAllSixButtons();
}
