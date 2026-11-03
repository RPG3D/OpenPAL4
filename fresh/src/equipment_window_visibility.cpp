#include "pal4/equipment_window_visibility.h"

namespace pal4 {

std::uint32_t EquipmentWindowVisibility::SetVisible(
    EquipmentWindowState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    IEquipmentWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                         "show_frame_tool", showing, 0u);
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                         "show_game_info", showing, state.Read(0x24u));
    runtime.SetEquipmentVisibility(state, false);
    runtime.ClearItemDetails(state);
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.SetSelected(state.Read(0x50u + index * 4u), false);
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto image = state.Read(0x98u + index * 4u);
        runtime.SetText(image, U"");
        runtime.SetEnabled(image, false);
    }
    if (showing != 0u) {
        state.Write(0xf4u, 4u);
        for (;;) {
            const auto next = runtime.NextCharacterIndex(state.Read(0xf4u));
            state.Write(0xf4u, next);
            if (next == 4u) break;
            const auto player = runtime.PlayerByIndex(next);
            if (player != 0u && runtime.ReadWord(player + 0xb08u) != 0u)
                break;
        }
        runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                             "set_help_tips_id", 7u, 0u);
        runtime.InsertActive(runtime.FrameManager() + 0x134u,
                             object_address + 0x28u);
        runtime.SelectCharacter(state, 5u);
        runtime.UpdateSystemDialogVisibility(runtime.FrameManager(), 3u);
    } else {
        runtime.ToggleModelVisibility(state, false);
        runtime.RemoveActive(runtime.FrameManager() + 0x134u,
                             object_address + 0x28u);
    }
    return runtime.RefreshMember(object_address + 0x118u);
}

}  // namespace pal4
