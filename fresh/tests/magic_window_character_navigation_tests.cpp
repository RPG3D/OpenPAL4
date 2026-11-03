#include "pal4/magic_window_character_navigation.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowCharacterNavigationRuntimeStub {
    std::vector<std::int32_t> indices;
    std::vector<std::int32_t> step_inputs;
    std::vector<std::int32_t> player_queries;
    std::vector<std::string> calls;
    std::int32_t active_index{-1};
    std::int32_t fallback_index{2};
    std::int32_t display_index{-1};

    std::int32_t Step(std::int32_t current, const char* direction) {
        step_inputs.push_back(current);
        calls.push_back(direction);
        if (indices.empty()) return fallback_index;
        const auto result = indices.front();
        indices.erase(indices.begin());
        return result;
    }
    std::int32_t PreviousIndex(std::int32_t current) override {
        return Step(current, "previous");
    }
    std::int32_t NextIndex(std::int32_t current) override {
        return Step(current, "next");
    }
    bool HasActivePlayer(std::int32_t index) override {
        player_queries.push_back(index);
        calls.push_back("player");
        return index == active_index;
    }
    void UpdateCharacterDisplay(std::uint32_t object,
                                std::int32_t index) override {
        Check(object == 0x3000u, "display owner");
        display_index = index;
        calls.push_back("display");
    }
};

void TestPrevious() {
    Runtime runtime;
    runtime.indices = {1, 0, 3};
    runtime.active_index = 3;
    pal4::MagicWindowState state;
    state.selected_character_index = 2;
    Check(pal4::MagicWindowCharacterNavigation::Previous(0x3000u, state, runtime),
          "previous handler returns true");
    Check(runtime.step_inputs == std::vector<std::int32_t>{2, 1, 0} &&
              runtime.player_queries == std::vector<std::int32_t>{1, 0, 3} &&
              state.selected_character_index == 3 && runtime.display_index == 3 &&
              runtime.calls.back() == "display",
          "previous skips inactive players and refreshes active one");
}

void TestNext() {
    Runtime runtime;
    runtime.indices = {3, 0, 1};
    runtime.active_index = 1;
    pal4::MagicWindowState state;
    state.selected_character_index = 2;
    Check(pal4::MagicWindowCharacterNavigation::Next(0x3000u, state, runtime),
          "next handler returns true");
    Check(runtime.step_inputs == std::vector<std::int32_t>{2, 3, 0} &&
              runtime.player_queries == std::vector<std::int32_t>{3, 0, 1} &&
              state.selected_character_index == 1 && runtime.display_index == 1,
          "next skips inactive players and refreshes active one");
}

void TestFullCycle() {
    for (const auto previous : {false, true}) {
        Runtime runtime;
        runtime.indices = {0, 1, 2};
        pal4::MagicWindowState state;
        state.selected_character_index = 2;
        const auto result = previous
            ? pal4::MagicWindowCharacterNavigation::Previous(0x3000u, state, runtime)
            : pal4::MagicWindowCharacterNavigation::Next(0x3000u, state, runtime);
        Check(result && state.selected_character_index == 2 &&
                  runtime.player_queries == std::vector<std::int32_t>{0, 1} &&
                  runtime.display_index == -1 && runtime.calls.back() ==
                      (previous ? "previous" : "next"),
              "returning to starting index avoids player lookup and display");
    }
    Runtime only;
    only.indices = {2};
    pal4::MagicWindowState state;
    state.selected_character_index = 2;
    Check(pal4::MagicWindowCharacterNavigation::Next(0x3000u, state, only) &&
              only.player_queries.empty() && only.display_index == -1,
          "single-character cycle exits immediately");
}

void TestNextCharacterIndex() {
    for (std::int32_t index = 0; index < 3; ++index)
        Check(pal4::MagicWindowCharacterNavigation::NextCharacterIndex(index) ==
                  index + 1,
              "regular next index");
    Check(pal4::MagicWindowCharacterNavigation::NextCharacterIndex(3) == 0 &&
              pal4::MagicWindowCharacterNavigation::NextCharacterIndex(4) == 0,
          "index above final slot wraps to zero");
    Check(pal4::MagicWindowCharacterNavigation::NextCharacterIndex(-1) == 0 &&
              pal4::MagicWindowCharacterNavigation::NextCharacterIndex(-2) == -1 &&
              pal4::MagicWindowCharacterNavigation::NextCharacterIndex(
                  std::numeric_limits<std::int32_t>::max()) ==
                  std::numeric_limits<std::int32_t>::min(),
          "signed 32-bit arithmetic matches the x86 helper");
}

}  // namespace

int main() {
    TestPrevious();
    TestNext();
    TestFullCycle();
    TestNextCharacterIndex();
}
