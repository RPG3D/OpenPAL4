#include "pal4/magic_window_category_events.h"

namespace pal4 {

bool MagicWindowCategoryEvents::HandleStuntClick(
    std::uint32_t object, std::uint32_t clicked_button,
    MagicWindowState& state, const MagicWindowPanelState& panel,
    IMagicWindowCategoryEventsRuntime& runtime) {
    state.category_position = runtime.ButtonKey(clicked_button);
    return SelectStunt(object, state, panel, runtime);
}

bool MagicWindowCategoryEvents::SelectStunt(
    std::uint32_t object, const MagicWindowState& state,
    const MagicWindowPanelState& panel,
    IMagicWindowCategoryEventsRuntime& runtime) {
    runtime.RemoveActive(object + 0x4cu);
    runtime.RemoveActive(object + 0x40u);
    for (std::uint32_t index = 0; index < 6u; ++index)
        runtime.SetSelected(panel.At(0xa8u + 4u * index), false);
    runtime.SetSelected(panel.At(0xa8u + 4u * state.category_position), true);
    runtime.UpdateCharacterDisplay(object, state.selected_character_index);
    runtime.InsertActive(object + (state.category_position == 0u ? 0x4cu : 0x40u));
    return true;
}

bool MagicWindowCategoryEvents::HandleMagicHover(
    std::uint32_t object, std::uint32_t hovered_button,
    IMagicWindowCategoryEventsRuntime& runtime) {
    runtime.UpdateMagicDetails(object,
                               static_cast<std::int32_t>(runtime.ButtonKey(hovered_button)));
    return true;
}

bool MagicWindowCategoryEvents::HandleStuntHover(
    std::uint32_t object, std::uint32_t hovered_button,
    const MagicWindowPanelState& panel,
    IMagicWindowCategoryEventsRuntime& runtime) {
    for (std::uint32_t index = 0; index < 7u; ++index) {
        if (panel.At(0x108u + 4u * index) == hovered_button) {
            runtime.UpdateStuntDetails(object, static_cast<std::int32_t>(index));
            break;
        }
    }
    return true;
}

bool MagicWindowCategoryEvents::HandleMagicButtonClick(
    std::uint32_t object, std::uint32_t event_address,
    std::uint32_t clicked_button, std::uint32_t event_guard,
    IMagicWindowCategoryEventsRuntime& runtime) {
    if (event_guard == 0) {
        const auto key = runtime.ButtonKey(clicked_button);
        const auto record = runtime.FindStuntRecord(key);
        runtime.HandleMagicSelection(object, event_address + 0xcu, record);
    }
    return true;
}

}  // namespace pal4
