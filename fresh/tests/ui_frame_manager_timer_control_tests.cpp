#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerTimerControlRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t frame = 0x7000u;

    std::uint32_t FindFrame(std::uint32_t manager,
                            const char* name) override {
        Check(manager == 0x5000u && std::string(name) == "portrait",
              "all controls find the exact portrait frame");
        events.emplace_back("find");
        return frame;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x70f0u, "duration field address");
        events.emplace_back("write");
        words[address] = value;
    }
    void ClearTimerAndText(std::uint32_t receiver) override {
        Check(receiver == 0x7000u, "clear receiver");
        events.emplace_back("clear");
    }
};

void CheckDuration() {
    Runtime missing;
    missing.frame = 0u;
    Check(pal4::UiFrameManagerSingleton::SetPortraitTimerDuration(
              0x5000u, 42, missing) == 0 &&
              missing.events == std::vector<std::string>{"find"},
          "missing frame returns zero without reading the flag");

    Runtime negative;
    Check(pal4::UiFrameManagerSingleton::SetPortraitTimerDuration(
              0x5000u, -7, negative) == -7 &&
              negative.events == std::vector<std::string>{"find"},
          "negative duration returns unchanged without flag read");
    negative.events.clear();
    Check(pal4::UiFrameManagerSingleton::SetPortraitTimerDuration(
              0x5000u, 0, negative) == 0 &&
              negative.events == std::vector<std::string>{"find"},
          "zero duration skips the flag");

    Runtime disabled;
    Check(pal4::UiFrameManagerSingleton::SetPortraitTimerDuration(
              0x5000u, 123, disabled) == 123 &&
              disabled.events ==
                  std::vector<std::string>{"find", "read"} &&
              disabled.words[0x70f0u] == 0u,
          "zero flag preserves old duration");

    Runtime enabled;
    enabled.words[0x70e8u] = 1u;
    Check(pal4::UiFrameManagerSingleton::SetPortraitTimerDuration(
              0x5000u, 123, enabled) == 123 &&
              enabled.words[0x70f0u] == 123u &&
              enabled.events ==
                  std::vector<std::string>{"find", "read", "write"},
          "positive duration writes only when enabled");
}

void CheckClearAndGet() {
    Runtime missing;
    missing.frame = 0u;
    pal4::UiFrameManagerSingleton::ClearPortraitTimerText(
        0x5000u, missing);
    Check(pal4::UiFrameManagerSingleton::GetPortraitTimerDuration(
              0x5000u, missing) == 0 &&
              missing.events ==
                  std::vector<std::string>{"find", "find"},
          "missing frame skips clear and reads");

    Runtime present;
    present.words[0x70f0u] = 0xffffffffu;
    pal4::UiFrameManagerSingleton::ClearPortraitTimerText(
        0x5000u, present);
    Check(pal4::UiFrameManagerSingleton::GetPortraitTimerDuration(
              0x5000u, present) == -1 &&
              present.events == std::vector<std::string>{
                  "find", "clear", "find", "read"},
          "present frame clears and reads signed duration");
}

}  // namespace

int main() {
    try {
        CheckDuration();
        CheckClearAndGet();
        std::cout << "ui_frame_manager_timer_control_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
