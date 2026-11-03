#include "pal4/script_select_dialog.h"

namespace pal4 {

std::int32_t ScriptSelectDialog::AddItem(
    std::uint32_t string_object, IScriptSelectDialogRuntime& runtime) {
    return runtime.AddButton(runtime.SelectDialog(runtime.UiFrameManager()),
                             string_object);
}

std::int32_t ScriptSelectDialog::LastSelection(
    IScriptSelectDialogRuntime& runtime) {
    const auto dialog = runtime.SelectDialog(runtime.UiFrameManager());
    return static_cast<std::int32_t>(runtime.ReadWord(dialog + 0x9cu));
}

std::uint32_t ScriptSelectDialog::SetDefaultSelection(
    const std::int32_t selection, IScriptSelectDialogRuntime& runtime) {
    const auto dialog = runtime.SelectDialog(runtime.UiFrameManager());
    runtime.WriteWord(dialog + 0xa0u,
                      static_cast<std::uint32_t>(selection));
    return dialog;
}

}  // namespace pal4
