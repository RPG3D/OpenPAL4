#include "pal4/load_window_save_selection.h"

namespace pal4 {
namespace {

void StoreAttribute(std::uint32_t document, const char* name,
                    std::uint32_t value, std::uint8_t slot_low_byte,
                    ILoadWindowSaveSelectionRuntime& runtime) {
    const auto key = runtime.InitializeKey(name, slot_low_byte);
    LoadWindowAttributePosition position{};
    try {
        position = runtime.FindAttribute(document, key);
    } catch (...) {
        runtime.ReleaseKey(key);
        throw;
    }
    runtime.ReleaseKey(key);
    if (position.current != position.end &&
        position.current != 0xffffffe4u) {
        runtime.SetAttributeInt(position.current + 0x1cu, value);
        return;
    }

    const auto attribute = runtime.InitializeAttribute(name, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        throw;
    }
    runtime.DestroyAttribute(attribute);
}

}  // namespace

std::uint8_t LoadWindowSaveSelection::Confirm(
    LoadWindowState& state, std::uint32_t response,
    ILoadWindowSaveSelectionRuntime& runtime) {
    if (response != 1u) return 1u;
    const auto list = state.Read(0x54u);
    const auto item = runtime.HoverItem(list);
    if (item == 0u) return 1u;

    const auto slot = runtime.ReadWord(item + 0x1b8u);
    const auto global_for_slot = runtime.GlobalData();
    runtime.WriteWord(global_for_slot + 0x2cu, slot);
    const auto scrollbar = runtime.ScrollbarOf(list);
    const auto position = runtime.ReadWord(scrollbar + 0x4c0u);
    const auto global_for_position = runtime.GlobalData();
    runtime.WriteWord(global_for_position + 0x30u, position);
    runtime.SaveDialogSettings(runtime.GlobalData());
    runtime.HideWindow(state, 0u, 0u);
    runtime.ProcessUiEvents("close_main_ui", 0u, 0u);

    const auto document = runtime.InitializeXmlDocument();
    try {
        StoreAttribute(document, "PALGAMEIV_LOADING_TYPE", 1u,
                       static_cast<std::uint8_t>(slot), runtime);
        StoreAttribute(document, "PALGAMEIV_LOADING_ARCHIVE", slot,
                       static_cast<std::uint8_t>(slot), runtime);
        runtime.EnterGameState(6u, document);
    } catch (...) {
        runtime.DestroyXmlDocument(document);
        throw;
    }
    runtime.DestroyXmlDocument(document);
    return 1u;
}

}  // namespace pal4
