#include "pal4/ol_info_commands.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::OlInfoCommandRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t code = 1u;
    bool fail_process{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x6010u,
              "member event code is read from event +0x10");
        calls.emplace_back("read-code");
        return code;
    }
    std::uint32_t ConstructBaseEvent() override {
        calls.emplace_back("construct-event");
        return 0x7000u;
    }
    void DestroyBaseEvent(std::uint32_t event) override {
        Check(event == 0x7000u, "temporary base event is destroyed");
        calls.emplace_back("destroy-event");
    }
    void HideWindow(std::uint32_t object,
                    std::uint32_t showing,
                    std::uint32_t parent) override {
        Check(object == 0x2000u && showing == 0u && parent == 0u,
              "close hides containing olInfo window with two zeros");
        calls.emplace_back("hide");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void ProcessCommand(std::uint32_t tree,
                        std::string_view command,
                        pal4::OlInfoEventWords& words) override {
        Check(tree == 0x8104u && command == "show_logo_ui" &&
                  words.first == 1u && words.second == 0u,
              "close dispatches show_logo_ui through manager tree +0x104");
        calls.emplace_back("show-logo");
        if (fail_process) throw std::runtime_error("dispatch");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::OlInfoCommands::Close(0x2000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "hide", "manager", "show-logo"},
          "close hides panel before opening logo");
    runtime = Runtime{};
    runtime.code = 2u;
    Check(pal4::OlInfoCommands::OnMemberCloseEvent(
              0x2028u, 0x6000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read-code"},
          "other event codes do not close the panel");
    runtime = Runtime{};
    Check(pal4::OlInfoCommands::OnMemberCloseEvent(
              0x2028u, 0x6000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read-code", "construct-event", "hide", "manager",
                  "show-logo", "destroy-event"},
          "code one constructs base event and closes containing window");
    runtime = Runtime{};
    runtime.fail_process = true;
    try {
        (void)pal4::OlInfoCommands::OnMemberCloseEvent(
            0x2028u, 0x6000u, runtime);
        Check(false, "dispatch exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "destroy-event",
              "dispatch exception destroys temporary CEGUI event");
    }
    std::cout << "olInfo commands tests passed\n";
}
