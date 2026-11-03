#include "pal4/base_object_animation_selection.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectAnimationSelectionRuntime {
    std::vector<Event> events;
    bool different{};
    std::uint32_t set_result{0x1234u};

    std::uint32_t CurrentAnimationName(
        pal4::BaseObjectState&) override {
        events.emplace_back('N', 0u);
        return 0x7000u;
    }
    std::int32_t CompareIgnoreCase(
        std::uint32_t first, std::uint32_t second) override {
        if (first == 0x6000u && second == 0x8a6884u) {
            events.emplace_back('C', second);
            return 37;
        }
        Check(first == 0x7000u && second == 0x6000u,
              "animation selection compares current name to requested name");
        events.emplace_back('c', second);
        return different ? -1 : 0;
    }
    std::uint32_t SetCurrentAnimation(
        pal4::BaseObjectState&, std::uint32_t name) override {
        Check(name == 0x6000u,
              "plain setter receives requested name");
        events.emplace_back('S', name);
        return set_result;
    }
    std::uint32_t SetCurrentAnimationWithFlag(
        pal4::BaseObjectState&, std::uint32_t name,
        std::uint32_t flag) override {
        Check(name == 0x6000u && flag == 0x55u,
              "flagged setter receives name and flag");
        events.emplace_back('F', flag);
        return set_result;
    }
};

void CheckPlainSelection() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Check(pal4::BaseObjectAnimationSelection::CompareAndSet(
              state, 0x6000u, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'N', 0u},
                                                   {'c', 0x6000u}},
          "zero gate/vector and equal animation skip both extra comparison and setter");
    runtime.events.clear();
    runtime.different = true;
    Check(pal4::BaseObjectAnimationSelection::CompareAndSet(
              state, 0x6000u, runtime) == 0x1234u &&
              runtime.events == std::vector<Event>{{'N', 0u},
                                                   {'c', 0x6000u},
                                                   {'S', 0x6000u}},
          "different animation forwards plain setter result");

    for (auto offset : {0x40cu, 0x414u}) {
        runtime.events.clear();
        state.Write(offset, std::bit_cast<std::uint32_t>(-1.0f));
        Check(pal4::BaseObjectAnimationSelection::CompareAndSet(
                  state, 0x6000u, runtime) == 0x1234u &&
                  runtime.events == std::vector<Event>{{'C', 0x8a6884u},
                                                       {'N', 0u},
                                                       {'c', 0x6000u},
                                                       {'S', 0x6000u}},
              "nonzero X or Z triggers ignored C01 comparison first");
        state.Write(offset, 0u);
    }
    runtime.events.clear();
    state.Write(0x41cu, 0x1201u);
    Check(pal4::BaseObjectAnimationSelection::CompareAndSet(
              state, 0x6000u, runtime) == 0x1234u &&
              runtime.events.front() == Event{'C', 0x8a6884u},
          "only low byte of gate word controls C01 comparison");
    runtime.events.clear();
    state.Write(0x41cu, 0x1200u);
    state.Write(0x40cu,
                std::bit_cast<std::uint32_t>(
                    std::numeric_limits<float>::quiet_NaN()));
    Check(pal4::BaseObjectAnimationSelection::CompareAndSet(
              state, 0x6000u, runtime) == 0x1234u &&
              runtime.events.front() == Event{'C', 0x8a6884u},
          "unordered X comparison follows IDA's nonzero branch");
}

void CheckFlaggedSelection() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    state.Write(0x41cu, 1u);
    Check(pal4::BaseObjectAnimationSelection::CompareAndSetWithFlag(
              state, 0x6000u, 0x55u, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'N', 0u},
                                                   {'c', 0x6000u}},
          "flagged equal path returns one without reading movement gate");
    runtime.events.clear();
    runtime.different = true;
    Check(pal4::BaseObjectAnimationSelection::CompareAndSetWithFlag(
              state, 0x6000u, 0x55u, runtime) == 0x1234u &&
              runtime.events == std::vector<Event>{{'N', 0u},
                                                   {'c', 0x6000u},
                                                   {'F', 0x55u}},
          "flagged mismatch forwards setter result and argument");
}

}  // namespace

int main() {
    CheckPlainSelection();
    CheckFlaggedSelection();
}
