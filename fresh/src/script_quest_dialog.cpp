#include "pal4/script_quest_dialog.h"

namespace pal4 {

std::uint32_t IScriptQuestDialogRuntime::ResolveFrame() {
    return CreateQuestDialog(FrameManager());
}

std::uint32_t ScriptQuestDialog::Show(
    const char* text, IScriptQuestDialogRuntime& runtime) {
    const auto dialog = runtime.CreateQuestDialog(runtime.FrameManager());
    runtime.ShowText(dialog, text ? text : "");
    return ScriptFrameStateDocument::Enter(runtime);
}

std::int32_t ScriptQuestDialog::Result(
    IScriptQuestDialogRuntime& runtime) {
    const auto dialog = runtime.CreateQuestDialog(runtime.FrameManager());
    return runtime.ReadSignedWord(dialog + 0x4cu);
}

}  // namespace pal4
