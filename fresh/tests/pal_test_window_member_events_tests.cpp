#include "pal4/pal_test_window_member_events.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalTestWindowMemberEventRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t code{};
    bool fail_close{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x7010u, "event code read at +0x10");
        calls.emplace_back("read");
        return code;
    }
    std::uint8_t HandleKeyEvent(std::uint32_t member,
                                std::uint32_t event) override {
        Check(member == 0x6028u && event == 0x7000u,
              "other event forwarded to list handler");
        calls.emplace_back("key");
        return 7u;
    }
    void CloseWithDefaultEventArgs(std::uint32_t owner) override {
        Check(owner == 0x6000u, "close receives owning dialog");
        calls.emplace_back("close");
        if (fail_close) throw std::runtime_error("close");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::PalTestWindowMemberEvents::OnEvent(
              0x6028u, 0x7000u, runtime) == 7u &&
                  runtime.calls == std::vector<std::string>{"read", "key"},
          "non-close event returns inherited key-handler result");
    runtime.calls.clear();
    runtime.code = 1u;
    Check(pal4::PalTestWindowMemberEvents::OnEvent(
              0x6028u, 0x7000u, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{"read", "close"},
          "close event routes through dialog and returns one");
    runtime.calls.clear();
    runtime.fail_close = true;
    try {
        (void)pal4::PalTestWindowMemberEvents::OnEvent(
            0x6028u, 0x7000u, runtime);
        Check(false, "close failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{"read", "close"},
              "close failure does not invoke key handler");
    }
    std::cout << "Pal test window member event tests passed\n";
}
