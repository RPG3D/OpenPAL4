#include "pal4/load_window_save_list.h"

namespace pal4 {

namespace {

void ReleaseExtraText(SaveSlotExtras& extras,
                      ILoadWindowSaveListRuntime& runtime) {
    if (extras.text == 0u) return;
    const auto prefix = extras.text - 1u;
    const auto count = runtime.ReadTextPrefixByte(prefix);
    if (count == 0u || count == 0xffu) {
        runtime.FreeText(prefix);
    } else {
        runtime.WriteTextPrefixByte(prefix,
                                    static_cast<std::uint8_t>(count - 1u));
    }
    extras.text = 0u;
}

}  // namespace

void LoadWindowSaveList::Populate(
    LoadWindowState& state, std::int32_t page,
    std::uint32_t refresh, ILoadWindowSaveListRuntime& runtime) {
    const auto list = state.Read(0x54u);
    const auto scrollbar = runtime.ScrollbarOf(list);
    const auto item_size = runtime.ScrollbarPosition(scrollbar) * 0.25f;

    const auto first_global = runtime.GlobalData();
    const auto save_count = runtime.ReadSignedWord(first_global + 0x2cu);
    const auto second_global = runtime.GlobalData();
    const auto saved_scroll = runtime.ReadFloat(second_global + 0x30u);
    const auto remainder = save_count % 10;

    const auto mask = state.Read(0x5cu);
    const auto parent = runtime.ParentOf(mask);
    const auto button = state.Read(
        0x2cu + static_cast<std::uint32_t>(page) * 4u);
    if (parent != 0u) runtime.RemoveChild(parent, mask);
    runtime.AddChild(button, mask);
    runtime.SetClippedByParent(mask, false);
    runtime.SetXPosition(mask, 1u, -30.0f);
    runtime.SetYPosition(mask, 1u, -25.0f);
    runtime.CopyWindowText(state.Read(0x60u), button, 0x4cu);
    const auto button_id = runtime.WindowUserId(button);
    runtime.SetWindowUserId(mask, button_id);
    runtime.ResetList(list);

    SaveSlotData data{};
    for (std::int32_t index = 0; index < 10; ++index) {
        const auto slot = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(button_id * 10u) +
            static_cast<std::uint32_t>(index));
        SaveSlotExtras extras{};
        extras.flags = refresh & 0xffu;
        try {
            const auto loaded = runtime.LoadSaveFile(slot, 1u, data, extras);
            if (!loaded) data.words[12] = 0u;

            const auto storage = runtime.Allocate(0x208u);
            std::uint32_t item = 0u;
            if (storage != 0u) {
                try {
                    const auto description = loaded ?
                        runtime.DescribeSlot(slot) : 0u;
                    item = runtime.ConstructItem(storage, slot, data,
                                                 description, 1u, extras);
                } catch (...) {
                    runtime.FreeAllocation(storage);
                    throw;
                }
            }
            runtime.SetItemSize(item, item_size);
            runtime.AddRow(list, 0u);
            const auto row_count = runtime.RowCount(list);
            runtime.SetItem(list, item, 0u, row_count - 1u);
        } catch (...) {
            ReleaseExtraText(extras, runtime);
            throw;
        }
        ReleaseExtraText(extras, runtime);
    }

    if ((refresh & 0xffu) != 0u &&
        static_cast<std::uint32_t>(remainder) < runtime.RowCount(list)) {
        runtime.SetHoverRowIndex(list, remainder);
        runtime.SetScrollPosition(scrollbar, saved_scroll);
        runtime.RequestRedraw(list);
        return;
    }
    runtime.SetHoverRowIndex(list, 0);
}

}  // namespace pal4
