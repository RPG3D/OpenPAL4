#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowCraftTabRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t source_event = 0x2000u;
    std::uint32_t source_value = 2u;
    std::uint32_t event = 0u;
    std::uint32_t dialog_key = 0u;
    std::uint32_t mode = 0xffffffffu;
    std::uint8_t result = 0xa5u;
    std::string image;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == source_event + 0x10u) {
            calls.emplace_back("read-event");
            return source_value;
        }
        Check(address == object + 0x78u, "tab image offset");
        calls.emplace_back("read-image");
        return 0x3000u;
    }
    void SetImage(std::uint32_t target,
                  std::string_view imageset,
                  std::string_view name) override {
        Check(target == 0x3000u && imageset == "zi0",
              "tab image target and set");
        image = name;
        calls.emplace_back("set-image");
    }
    void ProcessUIEvent(std::uint32_t key,
                        std::uint32_t value) override {
        Check(key == 0x8b13a4u, "UI event key");
        event = value;
        calls.emplace_back("ui-event");
    }
    void UpdateSystemDialogVisibility(std::uint32_t key) override {
        dialog_key = key;
        calls.emplace_back("system-dialog");
    }
    void ShowEffectPanel(std::uint32_t owner) override {
        Check(owner == object, "effect panel owner");
        calls.emplace_back("show-effect");
    }
    void HideSelectionPanel(std::uint32_t owner) override {
        Check(owner == object, "selection panel owner");
        calls.emplace_back("hide-selection");
    }
    std::uint8_t OnCraftModeClick(std::uint32_t owner,
                                   std::uint32_t requested_mode) override {
        Check(owner == object, "mode click owner");
        mode = requested_mode;
        calls.emplace_back("mode-click");
        return result;
    }
};

using Handler = std::uint8_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::ISmithWindowCraftTabRuntime&);

void CheckTabs() {
    constexpr std::array<Handler, 3> handlers{
        &pal4::SmithWindow::OnRongzhuTabClick,
        &pal4::SmithWindow::OnDuanyeTabClick,
        &pal4::SmithWindow::OnZhulingTabClick,
    };
    constexpr std::array<std::string_view, 3> images{
        "rongzhu", "duanye", "zhuling"};
    constexpr std::array<std::uint32_t, 3> events{9u, 10u, 11u};
    constexpr std::array<std::uint32_t, 3> keys{5u, 15u, 16u};

    for (std::uint32_t index = 0u; index < handlers.size(); ++index) {
        Runtime runtime;
        Check(handlers[index](Runtime::object, 0x12345678u,
                              runtime) == 0xa5u &&
                  runtime.image == images[index] &&
                  runtime.event == events[index] &&
                  runtime.dialog_key == keys[index] &&
                  runtime.mode == index,
              "tab constants, ignored event and forwarded result");
        Check(runtime.calls == (std::vector<std::string>{
                  "read-image", "set-image", "ui-event",
                  "system-dialog", "show-effect", "hide-selection",
                  "mode-click"}),
              "tab operation order");
    }

    Runtime zero_result;
    zero_result.result = 0u;
    Check(pal4::SmithWindow::OnRongzhuTabClick(
              Runtime::object, 0u, zero_result) == 0u,
          "tab returns mode-click low byte unchanged");
}

void CheckDispatch() {
    constexpr std::array<std::string_view, 3> images{
        "rongzhu", "duanye", "zhuling"};
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        Runtime runtime;
        runtime.source_value = index + 2u;
        runtime.result = 0u;
        Check(pal4::SmithWindow::DispatchCraftTabEvent(
                  Runtime::object + 0x28u, Runtime::source_event,
                  runtime) == 1u &&
                  runtime.image == images[index] && runtime.mode == index &&
                  runtime.calls.front() == "read-event",
              "event dispatch composes with the matching tab handler");
    }
    Runtime unmatched;
    unmatched.source_value = 5u;
    Check(pal4::SmithWindow::DispatchCraftTabEvent(
              Runtime::object + 0x28u, Runtime::source_event,
              unmatched) == 0u &&
              unmatched.calls == std::vector<std::string>{"read-event"},
          "unmatched event has no tab effects");
}

}  // namespace

int main() {
    try {
        CheckTabs();
        CheckDispatch();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
