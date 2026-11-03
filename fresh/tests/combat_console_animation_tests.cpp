#include "pal4/combat_console.h"

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

constexpr std::uint32_t kConsole = 0x5000u;

struct Runtime final : pal4::ICombatConsoleAnimationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kConsole + 220u, 0x8100u},
        {kConsole + 224u + 2u * 4u, 0x8200u},
        {kConsole + 236u + 5u * 4u, 0x9500u}};
    std::vector<std::string> calls;
    std::uint32_t state = 0u;
    std::uint32_t image = 0u;
    float x = 0.0f;
    float y = 0.0f;

    std::uint32_t GameState() override {
        calls.emplace_back("state");
        return state;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "animation image slot");
        calls.emplace_back("image-slot");
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        if (address == kConsole + 44u) {
            calls.emplace_back("left");
            return 90.0f;
        }
        if (address == kConsole + 52u) {
            calls.emplace_back("right");
            return 345.0f;
        }
        Check(address == kConsole + 48u, "animation y field");
        calls.emplace_back("y");
        return 185.0f;
    }
    std::uint32_t SetPosition(std::uint32_t target, std::uint32_t mode,
                              float new_x, float new_y) override {
        Check(mode == 1u, "pixel positioning mode");
        image = target;
        x = new_x;
        y = new_y;
        calls.emplace_back("set-position");
        return 0xabcdef01u;
    }
};

void StateAndIndexGates() {
    Runtime first_blocked;
    first_blocked.state = 1u;
    Check(pal4::CombatConsoleWindow::AnimatePosition(
              kConsole, 2, 32.0f, first_blocked) == 1u &&
              first_blocked.calls == std::vector<std::string>{"state"},
          "first family blocked by game state one");
    Runtime second_blocked;
    Check(pal4::CombatConsoleWindow::AnimatePositionSecond(
              kConsole, 5, 32.0f, second_blocked) == 0u &&
              second_blocked.calls == std::vector<std::string>{"state"},
          "second family blocked by game state zero");
    Runtime first_limit;
    Check(pal4::CombatConsoleWindow::AnimatePosition(
              kConsole, 3, 32.0f, first_limit) == 3u &&
              first_limit.calls == std::vector<std::string>{"state"},
          "first family stops at three");
    Runtime second_limit;
    second_limit.state = 1u;
    Check(pal4::CombatConsoleWindow::AnimatePositionSecond(
              kConsole, 6, 32.0f, second_limit) == 6u &&
              second_limit.calls == std::vector<std::string>{"state"},
          "second family stops at six");
}

void InterpolationAndSlotFamilies() {
    for (const auto [progress, expected_x] :
         {std::pair{0.0f, 90.0f}, std::pair{32.0f, 217.5f},
          std::pair{64.0f, 345.0f}}) {
        Runtime first;
        first.state = 2u;
        Check(pal4::CombatConsoleWindow::AnimatePosition(
                  kConsole, 2, progress, first) == 0xabcdef01u &&
                  first.image == 0x8200u && first.x == expected_x &&
                  first.y == 185.0f,
              "first family interpolates across one sixty-fourth scale");
        Check(first.calls == std::vector<std::string>{
                  "state", "image-slot", "left", "right", "y",
                  "set-position"},
              "interpolation read and set order");
    }
    Runtime negative;
    Check(pal4::CombatConsoleWindow::AnimatePosition(
              kConsole, -1, 64.0f, negative) == 0xabcdef01u &&
              negative.image == 0x8100u,
          "negative index passes signed upper-bound gate");

    Runtime second;
    second.state = 1u;
    Check(pal4::CombatConsoleWindow::AnimatePositionSecond(
              kConsole, 5, 32.0f, second) == 0xabcdef01u &&
              second.image == 0x9500u && second.x == 217.5f &&
              second.y == 185.0f,
          "second family uses +236 slot base");
}

}  // namespace

int main() {
    try {
        StateAndIndexGates();
        InterpolationAndSlotFamilies();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
