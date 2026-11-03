#include "pal4/magic_window_visibility.h"

namespace pal4 {

void MagicWindowVisibility::Set(
    std::uint32_t object, std::uint8_t showing,
    std::uint32_t parent, MagicWindowState& state,
    const MagicWindowPanelState& panel,
    IMagicWindowVisibilityRuntime& runtime) {
    const auto visible = showing != 0;
    if (visible) {
        runtime.SetEnabled(panel.At(0xa8u), runtime.HasGameDataEntry(19u));
        const auto magic_enabled = runtime.HasGameDataEntry(20u);
        for (std::uint32_t index = 1; index < 6u; ++index)
            runtime.SetEnabled(panel.At(0xa8u + 4u * index), magic_enabled);
    }

    runtime.SetParent(object, showing, parent);
    if (visible) {
        state.category_position = 6u;
        for (std::uint32_t index = 0; index < 6u; ++index)
            runtime.SetSelected(panel.At(0xa8u + 4u * index), false);
        runtime.SetStuntPanelVisible(object, false);
        state.selected_character_index = 4;
        for (;;) {
            state.selected_character_index =
                runtime.NextCharacterIndex(state.selected_character_index);
            if (state.selected_character_index == 4 ||
                runtime.HasActivePlayer(state.selected_character_index))
                break;
        }
    }

    runtime.ProcessEvent(kVisibilityEvent, showing, 0u);
    runtime.ProcessEvent(kOwnerEvent, showing, runtime.ReadWord(object + 0x24u));
    if (visible) {
        runtime.ProcessEvent(kHelpEvent, 8u, 0u);
        runtime.InsertActive(object + 0x28u);
        state.category_position = 0u;
        runtime.SelectStunt(object);
        runtime.UpdateSystemDialogVisibility(4u);
    } else {
        runtime.RemoveActive(object + 0x28u);
    }
}

}  // namespace pal4
