#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace pal4 {
// Per-call snapshot of actual actor state. Text is original game encoding;
// the UI layer converts it through the existing native game-text converter.
struct NativeCombatTooltipData {
    std::uint32_t actor{};
    std::uint32_t system{},camera{};
    bool monster{};
    std::string text;
    std::vector<std::int32_t> permanent,timed;
    std::int32_t maximum_hp{},hp{};
    std::array<float,2> actor_ui{};
};
}
