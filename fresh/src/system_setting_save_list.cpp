#include "pal4/system_setting_save_list.h"

#include <bit>

namespace pal4 {

void SystemSettingSaveList::Populate(
    const std::uint32_t frame, const std::int32_t page,
    const std::uint32_t unused_mode,
    ISystemSettingSaveListRuntime& runtime) {
    (void)unused_mode;
    const auto list = runtime.ReadWord(frame + 104u);
    const auto scrollbar = runtime.ReadWord(list + 1204u);
    const auto row_height = std::bit_cast<float>(
        runtime.ReadWord(scrollbar + 1204u)) * 0.25f;
    const auto slot = static_cast<std::int32_t>(
        runtime.ReadWord(runtime.GlobalData() + 44u));
    const auto scroll_position = std::bit_cast<float>(
        runtime.ReadWord(runtime.GlobalData() + 48u));
    const auto remainder = slot % 10;
    runtime.ResetList(list);
    while (runtime.ColumnCount(list) != 0u) {
        runtime.RemoveColumn(list, 0u);
    }
    const auto option = runtime.ReadWord(frame + 504u);
    const bool select_current_row = option != 1u;
    runtime.AddColumn(list, 0u, 1.0f);

    auto current_slot = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(page) * 10u);
    for (std::uint32_t index = 0; index < 10u; ++index) {
        SaveDataWords data{};
        SaveExtraWords extra{};
        try {
            const bool loaded = runtime.LoadSaveFile(current_slot, 1u,
                                                     data, extra);
            if (!loaded) {
                data[12] = 0u;
            }
            const auto allocation = runtime.Allocate(0x208u);
            std::uint32_t item = 0u;
            if (allocation != 0u) {
                const auto metadata = loaded ? runtime.SlotMetadata(current_slot)
                                             : 0u;
                item = runtime.ConstructItem(allocation, current_slot, data,
                                             metadata, option == 2u, extra);
            }
            runtime.SetItemSize(item, row_height);
            runtime.AddRow(list, 0u);
            const auto count = runtime.RowCount(list);
            runtime.SetItem(list, item, 0u, count - 1u);
        } catch (...) {
            runtime.ReleaseExtra(extra);
            throw;
        }
        runtime.ReleaseExtra(extra);
        current_slot = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(current_slot) + 1u);
    }

    if (select_current_row && remainder >= 0 &&
        static_cast<std::uint32_t>(remainder) < runtime.RowCount(list)) {
        runtime.SetHoverRow(list, static_cast<std::uint32_t>(remainder));
        runtime.SetScrollPosition(scrollbar, scroll_position);
    } else {
        runtime.SetHoverRow(list, 0u);
    }
    runtime.RequestRedraw(list);

    const auto mask = runtime.ReadWord(frame + 176u);
    const auto parent = runtime.ReadWord(mask + 68u);
    const auto button = runtime.ReadWord(frame + 136u +
        4u * static_cast<std::uint32_t>(page));
    if (parent != 0u) {
        runtime.RemoveChild(parent, mask);
    }
    runtime.WriteWord(mask + 280u, static_cast<std::uint32_t>(page));
    runtime.AddChild(button, mask);
    runtime.SetClippedByParent(mask, false);
    runtime.SetXPosition(mask, 1u,
                         static_cast<std::uint32_t>(-1041235968));
    runtime.SetYPosition(mask, 1u,
                         static_cast<std::uint32_t>(-1043857408));
    runtime.CopyText(runtime.ReadWord(frame + 108u), button);
}

}  // namespace pal4
