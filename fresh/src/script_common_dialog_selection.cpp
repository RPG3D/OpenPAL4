#include "pal4/script_common_dialog_selection.h"

namespace pal4 {

std::uint32_t ScriptCommonDialogSelection::IsOne(
    IScriptCommonDialogSelectionRuntime& runtime) {
    const auto dialog = runtime.CreateCommonDialog(runtime.FrameManager());
    return runtime.LastSelection(dialog) == 1 ? 1u : 0u;
}

}  // namespace pal4
