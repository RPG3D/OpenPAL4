#include "pal4/timer_frame_actions.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITimerFrameActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x1070u, "choice field offset");
        calls.emplace_back("write:" + std::to_string(value));
        words[address] = value;
    }
    void SetFrameVisibility(std::uint32_t frame,
                            std::uint8_t visible,
                            std::uint32_t parent) override {
        Check(frame == 0x1000u && visible == 0u && parent == 0u,
              "action hides frame through virtual visibility slot");
        calls.emplace_back("hide");
    }
    void ForwardMemberEvent(std::uint32_t member,
                            std::uint32_t event) override {
        Check(member == 0x1028u && event == 0x2000u,
              "other key events reach member +0x28");
        calls.emplace_back("forward");
    }
};

void CheckChoice(std::uint8_t (*action)(std::uint32_t,
                                       pal4::ITimerFrameActionsRuntime&),
                 std::uint32_t expected) {
    Runtime runtime;
    Check(action(0x1000u, runtime) == 1u &&
              runtime.words[0x1070u] == expected &&
              runtime.calls ==
                  std::vector<std::string>{
                      "write:" + std::to_string(expected), "hide"},
          "button writes choice before hiding frame");
}

}  // namespace

int main() {
    Runtime key_close;
    key_close.words[0x2010u] = 1u;
    Check(pal4::TimerFrameActions::OnButtonKey(
              0x1000u, 0x2000u, key_close) == 1u &&
              key_close.words[0x1070u] == 0u &&
              key_close.calls == std::vector<std::string>{
                                     "read", "write:0", "hide"},
          "event code one closes frame with zero choice");

    Runtime other_key;
    other_key.words[0x2010u] = 2u;
    Check(pal4::TimerFrameActions::OnButtonKey(
              0x1000u, 0x2000u, other_key) == 1u &&
              other_key.calls ==
                  std::vector<std::string>{"read", "forward"} &&
              other_key.words.count(0x1070u) == 0u,
          "other event code forwards without changing choice");

    CheckChoice(&pal4::TimerFrameActions::OnSettle, 1u);
    CheckChoice(&pal4::TimerFrameActions::OnDelegate, 2u);
    CheckChoice(&pal4::TimerFrameActions::OnExit, 0u);

    pal4::TimerCallbackState callback{};
    callback.reserved_04 = 0x1111u;
    callback.reserved_14 = 0x2222u;
    Check(&pal4::UiFrameEventCallback::Construct(
              callback, 0x47dbb0u, 0u, 0x1000u) == &callback &&
              callback.vtable == pal4::UiFrameEventCallback::kVtable &&
              callback.owner == 0x47dbb0u &&
              callback.function == 0u &&
              callback.argument == 0x1000u &&
              callback.reserved_04 == 0x1111u &&
              callback.reserved_14 == 0x2222u,
          "UI callback constructor writes +8/+12/+16 and vtable only");
}
