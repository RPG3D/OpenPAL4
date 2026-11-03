#include "pal4/magic_window_selection_handler.h"

namespace pal4 {

bool MagicWindowSelectionHandler::Handle(
    std::uint32_t event_payload, const MagicDbStuntRecord* record,
    const MagicWindowState& state,
    IMagicWindowRequirementsRuntime& requirements_runtime,
    IMagicWindowSelectionDialogRuntime& dialog_runtime) {
    if (record == nullptr)
        return true;

    dialog_runtime.HideAndCleanup();
    const auto enabled = MagicWindowRequirements::Check(
        state, *record, requirements_runtime);
    MagicSelectionButton button;
    button.record_key = record->key;
    button.command_address = kButtonCommandAddress;
    button.text = dialog_runtime.TriggerTextUtf8(0x16u);
    button.enabled = enabled;
    dialog_runtime.CreateButton(button);
    dialog_runtime.ShowDialog(event_payload);
    return true;
}

}  // namespace pal4
