#include "pal4/property_window_magic_item.h"

namespace pal4 {

bool PropertyWindowMagicItem::Use(PropertyWindowState& state,
                                  IPropertyWindowMagicItemRuntime& runtime) {
    if (!runtime.IsDialogVisible(state)) return true;

    runtime.ToggleModel(state, true);
    const auto entry = runtime.FindInventoryEntry(3182u, 5u);
    const auto category = state.Read(0x6cu);
    if (entry == 0u) {
        runtime.SetProperty(category, 0u);
        runtime.ShowOrHideDialog(0u, 0u, 0u);
    } else {
        runtime.SetProperty(category, runtime.ReadWord(entry + 4u));
    }
    runtime.PopulateList(state, category, false);
    return true;
}

}  // namespace pal4
