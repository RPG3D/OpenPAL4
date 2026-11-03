#include "pal4/magic_window_stunt_selection.h"

namespace pal4 {

bool MagicWindowStuntSelection::Handle(
    std::uint32_t object, std::uint32_t stunt_key,
    const MagicWindowState& state,
    IMagicWindowStuntSelectionRuntime& runtime) {
    if (!runtime.HasStunt(stunt_key))
        return true;
    const auto player = runtime.GetPlayer(state.selected_character_index);
    if (player == 0)
        return true;

    const auto stunt = runtime.FindStunt(stunt_key);
    if (stunt->required_stat > runtime.PlayerValue(player, 8u)) {
        const auto text = runtime.TriggerText(0x6cu);
        runtime.ShowStatFailure(text, 0.5f, 0.4f, 4u, 0u);
        return true;
    }

    auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0) {
        runtime.WriteWord(callback + 8u, kCastCallbackHandler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object);
        runtime.WriteWord(callback, kCastCallbackVtable);
    }
    try {
        runtime.ShowCastDialog(true, callback, *stunt);
    } catch (...) {
        if (callback != 0) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0) runtime.DeleteCallback(callback, 1u);
    return true;
}

}  // namespace pal4
