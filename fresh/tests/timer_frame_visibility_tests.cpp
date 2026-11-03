#include "pal4/timer_frame_visibility.h"

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

struct Runtime final : pal4::ITimerFrameVisibilityRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {pal4::TimerFrameVisibility::kNameSource, 0x5000u}};
    std::vector<std::string> calls;
    std::uint32_t existing = 0x7000u;
    std::uint32_t after_registration = 0x7000u;
    std::uint32_t find_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == pal4::TimerFrameVisibility::kNameSource,
              "frame name source");
        calls.emplace_back("name");
        return words.at(address);
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t name) override {
        Check(manager == 0x6000u && name == 0x5000u,
              "manager and named timer frame lookup");
        calls.emplace_back("find");
        return find_count++ == 0u ? existing : after_registration;
    }
    void RegisterFrame(std::uint32_t manager,
                       std::uint32_t name) override {
        Check(manager == 0x6000u && name == 0x5000u,
              "missing frame registration");
        calls.emplace_back("register");
    }
    void SetFrameVisibility(std::uint32_t frame,
                            std::uint8_t visible,
                            std::uint32_t parent) override {
        Check(frame == 0x7000u && parent == 0u,
              "virtual visibility call receives frame and null parent");
        calls.emplace_back(visible != 0u ? "show" : "hide");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x7074u, "frame selection offset");
        calls.emplace_back("selection");
        words[address] = value;
    }
};

}  // namespace

int main() {
    Runtime existing;
    Check(pal4::TimerFrameVisibility::SetVisible(
              1u, 3u, existing) &&
              existing.words[0x7074u] == 3u &&
              existing.calls == std::vector<std::string>{
                                    "name", "manager", "find",
                                    "selection", "show"},
          "present frame gets selection then visible call");

    Runtime registered;
    registered.existing = 0u;
    Check(pal4::TimerFrameVisibility::SetVisible(
              1u, 2u, registered) &&
              registered.words[0x7074u] == 2u &&
              registered.calls == std::vector<std::string>{
                                      "name", "manager", "find",
                                      "manager", "register", "manager",
                                      "find", "selection", "show"},
          "missing frame is registered then found with fresh manager calls");

    Runtime failed;
    failed.existing = 0u;
    failed.after_registration = 0u;
    Check(!pal4::TimerFrameVisibility::SetVisible(
              1u, 1u, failed) &&
              failed.calls.back() == "find" &&
              failed.words.count(0x7074u) == 0u,
          "registration without a frame returns false without showing");

    Runtime hide;
    Check(pal4::TimerFrameVisibility::SetVisible(
              0u, 9u, hide) &&
              hide.words.count(0x7074u) == 0u &&
              hide.calls == std::vector<std::string>{
                                "name", "manager", "find", "hide"},
          "hide keeps selection and does not register");

    Runtime absent;
    absent.existing = 0u;
    Check(!pal4::TimerFrameVisibility::SetVisible(
              0u, 0u, absent) &&
              absent.calls == std::vector<std::string>{
                                  "name", "manager", "find"},
          "hide with absent frame only looks up");
}
