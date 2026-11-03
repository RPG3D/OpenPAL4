#include "pal4/property_window_discard.h"

namespace pal4 {

bool PropertyWindowDiscard::Show(
    PropertyWindowState& state, std::uint32_t event,
    IPropertyWindowDiscardRuntime& runtime) {
    const auto item = runtime.ReadWord(event + 4u);
    const auto dialog = runtime.CreateCommonDialog();
    runtime.InitializeBuffer();
    std::uint32_t callback = 0u;
    try {
        const auto definition = runtime.ReadWord(item);
        const auto description_address = runtime.ReadWord(definition + 0x14u);
        const auto description = runtime.ReadText(
            description_address != 0u ? description_address : kEmptyTextAddress);
        const auto format_address = runtime.TriggerTextAddress(0x68u);
        const auto format = runtime.ReadText(
            format_address != 0u ? format_address : kEmptyTextAddress);
        const auto formatted = runtime.FormatBuffer(format, description);

        callback = runtime.Allocate(0x18u);
        if (callback != 0u) {
            runtime.WriteWord(callback + 8u, kConfirmationCallback);
            runtime.WriteWord(callback + 12u, 0u);
            runtime.WriteWord(callback + 16u, runtime.ObjectAddress(state));
            runtime.WriteWord(callback, kCallbackVtable);
        }
        const auto text = runtime.ConvertToUtf8(formatted);
        runtime.ShowWithFlags(dialog, text, 0.3f, 0.3f, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        runtime.CleanupBuffer();
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    runtime.CleanupBuffer();
    return true;
}

bool PropertyWindowDiscard::Confirm(
    PropertyWindowState& state, std::uint32_t choice,
    IPropertyWindowDiscardRuntime& runtime) {
    const auto record = runtime.HoveredRecord(state);
    if (record == 0u || choice != 1u) return true;
    runtime.PlayMusic("wa07.wav", 1u);
    const auto definition = runtime.ReadWord(record);
    const auto count = runtime.ReadWord(record + 8u);
    runtime.RemoveItem(runtime.ReadWord(definition), count);
    runtime.PopulateList(state, state.Read(0x6cu), false);
    return true;
}

}  // namespace pal4
