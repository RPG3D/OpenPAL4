#include "pal4/select_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const char* StateName(pal4::SelectButtonState state) {
    switch (state) {
    case pal4::SelectButtonState::Normal: return "normal";
    case pal4::SelectButtonState::Hover: return "hover";
    case pal4::SelectButtonState::Pushed: return "pushed";
    case pal4::SelectButtonState::Disabled: return "disabled";
    }
    throw std::runtime_error("unknown state");
}

struct Runtime final : pal4::ISelectDialogButtonRuntime {
    static constexpr std::uint32_t kObject = 0x1000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t id = 17u;
    std::uint32_t callback_allocation = 0x7000u;
    bool destroyed = false;

    Runtime() {
        words[kObject + 140u] = 0x2000u;
        words[kObject + 148u] = 0x3000u;
        words[0x2000u + 0x6b0u] = 0x4000u;
        words[0x8000u + 4u] = 1u;
        words[0x5000u + 4u] = 0x12345678u;
    }
    std::uint32_t NextButtonId() override {
        events.emplace_back("next-id");
        return id;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write-word");
    }
    std::uint32_t CreateWindow(const char* type,
                               const std::string& name) override {
        const auto button = std::string(type) == "OiramLook/Button";
        Check(button ? name == "SelectDialogBtn_17" :
                       std::string(type) == "OiramLook/StaticImage" &&
                       name == "SelectDialogPanel_17",
              "generated type and name");
        events.emplace_back(button ? "create-button" :
                                     "create-panel");
        return button ? 0x5000u : 0x6000u;
    }
    void CopyTextColour(std::uint32_t button,
                        std::uint32_t template_window,
                        pal4::SelectButtonState state) override {
        Check(button == 0x5000u &&
                  template_window == 0x3000u,
              "text colour windows");
        events.emplace_back(std::string("colour-") +
                            StateName(state));
    }
    std::uint32_t GetStateImage(
        std::uint32_t template_window,
        pal4::SelectButtonState state) override {
        Check(template_window == 0x3000u,
              "image template window");
        events.emplace_back(std::string("get-") +
                            StateName(state));
        return 0x8000u +
               static_cast<std::uint32_t>(state);
    }
    void SetStateImage(std::uint32_t button,
                       pal4::SelectButtonState state,
                       std::uint32_t image) override {
        Check(button == 0x5000u &&
                  image == 0x8000u +
                               static_cast<std::uint32_t>(state),
              "state image binding");
        events.emplace_back(std::string("set-") +
                            StateName(state));
    }
    void SetPanelImage(std::uint32_t panel,
                       std::uint32_t image) override {
        Check(panel == 0x6000u && image == 0x4000u,
              "panel inherits body image");
        events.emplace_back("panel-image");
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        Check(parent == 0x6000u && child == 0x5000u,
              "button nested in panel");
        events.emplace_back("add-child");
    }
    pal4::SelectPosition GetMetricPosition(
        std::uint32_t window, std::uint32_t mode) override {
        Check(window == 0x3000u && mode == 1u,
              "template metric position");
        events.emplace_back("get-position");
        return {0x3f800000u, 0x3e800000u};
    }
    void SetMetricPosition(std::uint32_t window,
                           std::uint32_t mode,
                           pal4::SelectPosition position) override {
        Check(window == 0x5000u && mode == 1u &&
                  position.x == 0x3f800000u &&
                  position.y == 0x3e800000u,
              "button metric position");
        events.emplace_back("set-position");
    }
    float GetMetricHeight(std::uint32_t window,
                          std::uint32_t mode) override {
        Check(window == 0x3000u && mode == 1u,
              "template metric height");
        events.emplace_back("get-height");
        return 0.2f;
    }
    float GetMetricWidth(std::uint32_t window,
                         std::uint32_t mode) override {
        Check(window == 0x3000u && mode == 1u,
              "template metric width");
        events.emplace_back("get-width");
        return 0.8f;
    }
    void SetMetricHeight(std::uint32_t window,
                         std::uint32_t mode,
                         float height) override {
        Check(window == 0x5000u && mode == 1u &&
                  height == 0.2f,
              "button metric height");
        events.emplace_back("set-height");
    }
    void SetMetricWidth(std::uint32_t window,
                        std::uint32_t mode,
                        float width) override {
        Check(window == 0x5000u && mode == 1u &&
                  width == 0.8f,
              "button metric width");
        events.emplace_back("set-width");
    }
    std::uint32_t AllocateCallback(
        std::uint32_t size) override {
        Check(size == 0x18u, "click callback size");
        events.emplace_back("allocate-callback");
        return callback_allocation;
    }
    void InitializeClickCallback(std::uint32_t allocation,
                                 std::uint32_t owner,
                                 std::uint32_t handler) override {
        Check(allocation == 0x7000u && owner == kObject &&
                  handler == pal4::SelectDialog::kClickCallback,
              "click callback fields");
        events.emplace_back("init-callback");
    }
    std::uint32_t SubscribeClicked(std::uint32_t button,
                          std::uint32_t callback) override {
        Check(button == 0x5000u &&
                  callback == callback_allocation,
              "button click subscription");
        events.emplace_back("subscribe");
        return 0x8000u;
    }
    void DestroyWindowOwner(std::uint32_t window,
                            std::uint32_t flags) override {
        Check(window == 0x8000u && flags == 1u,
              "button owner release");
        destroyed = true;
        events.emplace_back("destroy-owner");
    }
};

void CheckButton() {
    Runtime runtime;
    Check(pal4::SelectDialog::CreateButton(
              Runtime::kObject, runtime) == 0x6000u &&
              runtime.words.at(0x8000u + 4u) == 0u &&
              runtime.words.at(0x5000u + 4u) == 0x12345678u &&
              runtime.destroyed,
          "button creation return and owner release");
    Check(runtime.events == std::vector<std::string>{
              "next-id", "create-button", "colour-normal",
              "colour-hover", "colour-pushed",
              "colour-disabled", "get-normal", "set-normal",
              "get-hover", "set-hover", "get-disabled",
              "set-disabled", "get-pushed", "set-pushed",
              "create-panel", "panel-image", "add-child",
              "get-position", "set-position", "get-height",
              "set-height", "get-width", "set-width",
              "allocate-callback", "init-callback",
              "subscribe", "write-word", "destroy-owner"},
          "button construction order");
}

void CheckNullAllocationAndSharedOwner() {
    Runtime runtime;
    runtime.callback_allocation = 0u;
    runtime.words[0x8000u + 4u] = 3u;
    Check(pal4::SelectDialog::CreateButton(
              Runtime::kObject, runtime) == 0x6000u &&
              runtime.words.at(0x8000u + 4u) == 2u &&
              !runtime.destroyed,
          "null callback and retained button owner");
    Check(runtime.events[runtime.events.size() - 2u] ==
              "subscribe" &&
              runtime.events.back() == "write-word",
          "null callback is still subscribed");
}

}  // namespace

int main() {
    try {
        CheckButton();
        CheckNullAllocationAndSharedOwner();
        std::cout << "select_dialog_button_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
