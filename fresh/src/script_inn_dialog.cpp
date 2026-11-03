#include "pal4/script_inn_dialog.h"

namespace pal4 {

std::uint32_t IScriptInnDialogRuntime::ResolveFrame() {
    return FindFrame(FrameManager(), ScriptInnDialog::kFrameKeyCell);
}

std::uint32_t ScriptInnDialog::Show(
    const std::uint32_t enabled, IScriptInnDialogRuntime& runtime) {
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u, kEventKeyCell,
                         1u, enabled & 0xffu);
    return ScriptFrameStateDocument::Enter(runtime);
}

std::int32_t ScriptInnDialog::Result(IScriptInnDialogRuntime& runtime) {
    (void)runtime.FrameManager();
    const auto frame = runtime.FindFrame(runtime.FrameManager(), kFrameKeyCell);
    return runtime.ReadSignedWord(frame + 0x70u);
}

}  // namespace pal4
