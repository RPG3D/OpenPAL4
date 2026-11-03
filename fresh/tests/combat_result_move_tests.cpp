#include "pal4/combat_result_move.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultMoveRuntime {
    std::vector<std::string> calls;
    float y = 0.0f;
    float target = 10.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x403cu, "window field");
        calls.emplace_back("window");
        return 0x9000u;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x4040u, "target field");
        calls.emplace_back("target");
        return target;
    }
    float GetYPosition(std::uint32_t window) override {
        Check(window == 0x9000u, "get window");
        calls.emplace_back("get");
        return y;
    }
    void SetYPosition(std::uint32_t window, float value) override {
        Check(window == 0x9000u, "set window");
        calls.emplace_back("set");
        y = value;
    }
};

void MovementAndLimit() {
    Runtime runtime;
    Check(pal4::CombatResultMove::Update(0x4000u, 2.0f, runtime) == 1u &&
              runtime.y == 5.0f &&
              runtime.calls == std::vector<std::string>{
                                   "window", "get", "window", "set",
                                   "window", "get", "target"},
          "position below target stays at updated Y");
    runtime.calls.clear();
    Check(!pal4::CombatResultMove::Reached(0x4000u, runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"window", "get", "target"},
          "position below target has not arrived");
    runtime.calls.clear();
    Check(pal4::CombatResultMove::Update(0x4000u, 4.0f, runtime) == 1u &&
              runtime.y == 10.0f &&
              runtime.calls == std::vector<std::string>{
                                   "window", "get", "window", "set",
                                   "window", "get", "target", "target",
                                   "window", "set"},
          "overshoot is clamped after second position read");
    Check(pal4::CombatResultMove::Reached(0x4000u, runtime),
          "equal target is reached");
    runtime.y = 11.0f;
    Check(pal4::CombatResultMove::Reached(0x4000u, runtime),
          "above target is reached");
    runtime.y = std::numeric_limits<float>::quiet_NaN();
    Check(!pal4::CombatResultMove::Reached(0x4000u, runtime),
          "unordered position is not reached");
}

}  // namespace

int main() {
    try {
        MovementAndLimit();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
