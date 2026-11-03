#include "pal4/select_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint8_t SelectDialog::HandleNavigationKey(
    const std::uint32_t object, const std::uint32_t event,
    ISelectDialogEventRuntime& runtime, IUiNavigationInputRuntime& navigation) {
    if (runtime.ReadWord(event + 16u) != 1u)
        return UiNavigationList::HandleVerticalKey(object + 40u, event, navigation);
    runtime.WriteWord(object + 156u, runtime.ReadWord(object + 160u));
    runtime.HideAndClearList(object);
    const auto allocation = runtime.ReadWord(object + 64u);
    if (allocation != 0u)
        runtime.InvokeSelectionCallback(runtime.ReadWord(allocation), runtime.ReadWord(object + 156u));
    runtime.CleanupCallback(object);
    return 1u;
}

std::uint8_t SelectDialog::HandleKeyDown(
    const std::uint32_t object, const std::uint32_t event,
    ISelectDialogEventRuntime& runtime) {
    runtime.ForwardNavigationKey(object + 40u, event);
    return 1u;
}

std::uint8_t SelectDialog::HandleClick(
    const std::uint32_t object, const std::uint32_t event,
    ISelectDialogEventRuntime& runtime) {
    const auto window = runtime.ReadWord(event + 8u);
    const auto index = runtime.ReadWord(window + 280u);
    runtime.WriteWord(object + 156u, index);
    runtime.HideAndClearList(object);
    const auto allocation = runtime.ReadWord(object + 64u);
    if (allocation != 0u) {
        runtime.InvokeSelectionCallback(runtime.ReadWord(allocation),
                                         runtime.ReadWord(object + 156u));
    }
    runtime.CleanupCallback(object);
    return 1u;
}

}  // namespace pal4
