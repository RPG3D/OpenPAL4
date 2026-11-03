#include "pal4/dialog_manager_flags.h"

namespace pal4 {

void DialogManagerFlags::ClearAndSave(
    DialogManagerState& state, IDialogManagerFlagRuntime& game,
    IDialogManagerSettingsWriteRuntime& writer) {
    const auto bits = game.GameInstance() + 0x164;
    for (int index = 3; index >= 0; --index)
        game.SetGameWord(bits + static_cast<std::uint32_t>(index) * 4, 0);
    DialogManagerSettings::Save(state, writer);
}

void DialogManagerFlags::SetAndSave(
    DialogManagerState& state, const std::uint32_t position,
    IDialogManagerFlagRuntime& game,
    IDialogManagerSettingsWriteRuntime& writer) {
    static_cast<void>(game.GameInstance());
    if (position >= 128) return;
    const auto bits = game.GameInstance() + 0x164;
    const auto address = bits + (position >> 5) * 4;
    game.SetGameWord(address,
                     game.GameWord(address) | (1u << (position & 31)));
    DialogManagerSettings::Save(state, writer);
}

bool DialogManagerFlags::IsSet(const std::uint32_t position,
                               IDialogManagerFlagRuntime& game) {
    static_cast<void>(game.GameInstance());
    if (position >= 128) return false;
    const auto bits = game.GameInstance() + 0x164;
    return (game.GameWord(bits + (position >> 5) * 4) &
            (1u << (position & 31))) != 0;
}

}  // namespace pal4
