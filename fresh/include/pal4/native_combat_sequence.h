#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace pal4 {
// A frame-local copy of actual actor queries. No legacy container/string ABI.
struct NativeCombatSequenceActor {
    std::uint32_t address{},slot{};
    std::string image;
    float progress{};
    bool state_three{},state_zero{},ready{};
};
struct NativeCombatSequence {
    std::vector<NativeCombatSequenceActor> players,monsters,all_monsters;
};
}
