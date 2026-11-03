#include "pal4/magic_window_character_navigation.h"

#include <bit>

namespace pal4 {

std::int32_t MagicWindowCharacterNavigation::NextCharacterIndex(
    std::int32_t current) noexcept {
    const auto next = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(current) + 1u);
    return next > 3 ? 0 : next;
}

bool MagicWindowCharacterNavigation::Previous(
    std::uint32_t object, MagicWindowState& state,
    IMagicWindowCharacterNavigationRuntime& runtime) {
    const auto start = state.selected_character_index;
    for (;;) {
        state.selected_character_index =
            runtime.PreviousIndex(state.selected_character_index);
        if (state.selected_character_index == start)
            return true;
        if (runtime.HasActivePlayer(state.selected_character_index)) {
            runtime.UpdateCharacterDisplay(object, state.selected_character_index);
            return true;
        }
    }
}

bool MagicWindowCharacterNavigation::Next(
    std::uint32_t object, MagicWindowState& state,
    IMagicWindowCharacterNavigationRuntime& runtime) {
    const auto start = state.selected_character_index;
    for (;;) {
        state.selected_character_index =
            runtime.NextIndex(state.selected_character_index);
        if (state.selected_character_index == start)
            return true;
        if (runtime.HasActivePlayer(state.selected_character_index)) {
            runtime.UpdateCharacterDisplay(object, state.selected_character_index);
            return true;
        }
    }
}

}  // namespace pal4
