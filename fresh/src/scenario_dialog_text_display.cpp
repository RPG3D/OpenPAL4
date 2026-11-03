#include "pal4/scenario_dialog.h"

#include <cstdint>

namespace pal4 {

void ScenarioDialog::HandleTextDisplay(
    const std::uint32_t object,
    IScenarioTextDisplayRuntime& runtime) {
    const auto editbox = runtime.ReadWord(object + 56u);
    if (runtime.IsTypeEnd(editbox)) {
        runtime.HideAndClear(object);
        return;
    }
    if (runtime.NeedChangePage(editbox)) {
        runtime.ChangePage(editbox);
        return;
    }
    if (!runtime.NeedChangePage(editbox)) {
        do {
            runtime.ChangeLine(editbox);
        } while (!runtime.IsTypeEnd(editbox) &&
                 !runtime.NeedChangePage(editbox));
    }
    runtime.RequestRedraw(editbox);
}

}  // namespace pal4
