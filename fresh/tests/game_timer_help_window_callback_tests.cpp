#include "pal4/game_timer_help_window_callback.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameTimerHelpWindowRuntime {
    std::vector<int> calls;
    std::uint32_t counter = 0u;
    std::uint32_t frame = 0u;
    std::uint8_t result = 0u;
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x3000u, "callback counter address");
        calls.push_back(1);
        return counter;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x3000u, "callback write address");
        calls.push_back(2);
        counter = value;
    }
    std::uint32_t UiFrameManager() override {
        calls.push_back(3);
        return 0x4000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x4000u && name == "gameInfo",
              "gameInfo lookup operands");
        calls.push_back(4);
        return frame;
    }
    std::uint8_t UpdateClock(std::uint32_t target) override {
        Check(target == frame, "clock target");
        calls.push_back(5);
        return result;
    }
};

}  // namespace

int main() {
    try {
        const pal4::GameTimerHelpWindowCallback callback(0x3000u);
        Runtime runtime;
        runtime.counter = 0xffffffffu;
        Check(callback.Tick(runtime) == 0u && runtime.counter == 0u &&
                  runtime.calls == std::vector<int>{1, 2, 3, 4},
              "missing frame still increments counter with 32-bit wrap");
        runtime.calls.clear();
        runtime.frame = 0x5000u;
        runtime.result = 0xe3u;
        Check(callback.Tick(runtime) == 0xe3u && runtime.counter == 1u &&
                  runtime.calls == std::vector<int>{1, 2, 3, 4, 5},
              "present frame updates clock after increment");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
