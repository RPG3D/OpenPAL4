#include "pal4/ui_frame_event_handler.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameEventRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> finds;
    std::uint32_t manager = 0x7000u;
    std::uint32_t frame = 0u;
    std::uint32_t visible_frame = 0u;
    std::uint32_t visible_state = 99u;
    std::uint32_t visible_argument = 0u;
    std::uint32_t registered_name = 0u;

    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t FindFrame(std::uint32_t owner,
                            std::uint32_t name) override {
        calls.emplace_back("find");
        Check(owner == manager && name == 0x8100u,
              "frame lookup arguments");
        if (finds.empty()) return frame;
        const auto result = finds.front();
        finds.erase(finds.begin());
        return result;
    }
    void RegisterFrame(std::uint32_t owner,
                       std::uint32_t name) override {
        calls.emplace_back("register");
        Check(owner == manager, "frame registration manager");
        registered_name = name;
    }
    void SetVisible(std::uint32_t target, std::uint32_t enabled,
                    std::uint32_t argument) override {
        calls.emplace_back("visible");
        visible_frame = target;
        visible_state = enabled;
        visible_argument = argument;
    }
};

void CheckShowExisting() {
    Runtime runtime;
    runtime.frame = 0x9000u;
    Check(pal4::UiFrameEventHandler::Handle(
              0x8100u, {7u, 0xaabbccdd}, runtime) &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "find", "visible"}) &&
              runtime.visible_frame == 0x9000u &&
              runtime.visible_state == 1u &&
              runtime.visible_argument == 0xaabbccddu,
          "existing frame receives normalized show state and payload");
}

void CheckRegisterAndShow() {
    Runtime runtime;
    runtime.finds = {0u, 0x9000u};
    Check(pal4::UiFrameEventHandler::Handle(
              0x8100u, {1u, 42u}, runtime) &&
              runtime.calls ==
                  (std::vector<std::string>{"manager", "find",
                                            "manager", "register",
                                            "manager", "find", "visible"}) &&
              runtime.registered_name == 0x8100u &&
              runtime.visible_frame == 0x9000u &&
              runtime.visible_argument == 42u,
          "missing show frame registers then retries lookup");

    Runtime absent;
    absent.finds = {0u, 0u};
    Check(!pal4::UiFrameEventHandler::Handle(
              0x8100u, {1u, 9u}, absent) &&
              absent.calls.back() == "find" &&
              absent.visible_state == 99u,
          "show returns false when registration still yields no frame");
}

void CheckHide() {
    Runtime existing;
    existing.frame = 0x9000u;
    Check(pal4::UiFrameEventHandler::Handle(
              0x8100u, {0u, 19u}, existing) &&
              existing.calls ==
                  (std::vector<std::string>{"manager", "find", "visible"}) &&
              existing.visible_state == 0u &&
              existing.visible_argument == 19u,
          "hide forwards zero state and payload");

    Runtime absent;
    Check(!pal4::UiFrameEventHandler::Handle(
              0x8100u, {0u, 19u}, absent) &&
              absent.calls ==
                  (std::vector<std::string>{"manager", "find"}) &&
              absent.registered_name == 0u,
          "missing hide frame does not register a replacement");
}

}  // namespace

int main() {
    try {
        CheckShowExisting();
        CheckRegisterAndShow();
        CheckHide();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
