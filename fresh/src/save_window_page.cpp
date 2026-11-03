#include "pal4/save_window_page.h"

namespace pal4 {

void SaveWindowPage::Populate(SaveWindowState& state, std::int32_t page,
                              std::uint8_t reset,
                              ISaveWindowPageRuntime& runtime) {
    const auto list = state.Read(0x54u);
    const auto mask = state.Read(0x5cu);
    const auto item_size = runtime.ScrollbarExtent(list) * 0.25f;
    const auto current_slot = runtime.GlobalSaveIndex();
    const auto scroll_position = runtime.GlobalScrollPosition();
    const auto current_row = static_cast<std::uint32_t>(current_slot % 10);
    const auto button = state.Read(0x2cu + static_cast<std::uint32_t>(page) * 4u);
    const auto old_parent = runtime.ReadWord(mask + 0x44u);
    if (old_parent != 0u) runtime.RemoveChild(old_parent, mask);
    runtime.AddChild(button, mask);
    runtime.SetClippedByParent(mask, false);
    runtime.SetX(mask, 1u, -30.0f);
    runtime.SetY(mask, 1u, -25.0f);
    runtime.CopyButtonText(state.Read(0x60u), button);
    const auto displayed_page = runtime.ReadWord(button + 0x118u);
    runtime.WriteWord(mask + 0x118u, displayed_page);
    runtime.ResetList(list);

    auto slot = static_cast<std::int32_t>(displayed_page * 10u);
    for (std::uint32_t i = 0u; i < 10u; ++i, ++slot) {
        SaveListExtra extra{};
        extra.words[0] = reset;
        SaveSlotData data{};
        try {
            const bool loaded = runtime.LoadSave(slot, 1u, data, extra);
            if (!loaded) data.words[12] = 0u;
            const auto storage = runtime.AllocateItem(0x208u);
            std::uint32_t item = 0u;
            if (storage != 0u) {
                const auto auxiliary = loaded ? runtime.NumberedItemAux(slot) : 0u;
                item = runtime.ConstructItem(storage, slot, data,
                                             auxiliary, 0u, extra);
            }
            runtime.SetItemSize(item, item_size);
            runtime.AddRow(list, 0u);
            const auto row = runtime.RowCount(list) - 1u;
            runtime.SetItem(list, item, 0u, row);
        } catch (...) {
            runtime.ReleaseExtra(extra);
            throw;
        }
        runtime.ReleaseExtra(extra);
    }

    if (reset != 0u && current_row < runtime.RowCount(list)) {
        runtime.SetHoverRow(list, current_row);
        runtime.SetScrollPosition(list, scroll_position);
        runtime.RequestRedraw(list);
    } else {
        runtime.SetHoverRow(list, 0u);
    }
}

}  // namespace pal4
