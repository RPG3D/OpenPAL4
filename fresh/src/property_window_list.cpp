#include "pal4/property_window_list.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 6> kCategoryImages{{
    "huifu", "gongjiwupin", "fuzhuwuping", "cailiao", "juqing",
    "xinzenga",
}};

void AppendItem(std::uint32_t list, std::uint32_t row,
                const PropertyInventoryRecord& record,
                IPropertyWindowListRuntime& runtime) {
    runtime.AddRow(list);
    const auto name = runtime.ConvertToUtf8(record.name.value_or(""));
    const auto name_item = runtime.CreateItem(
        list, name, PropertyWindowList::kItemBytes,
        PropertyWindowList::kItemVtable);
    runtime.WriteItemId(name_item, record.id);
    runtime.SetItem(list, name_item, 0u, row);

    const auto count = runtime.FormatCount(
        static_cast<std::int32_t>(record.count));
    const auto count_text = runtime.ConvertToUtf8(count);
    const auto count_item = runtime.CreateItem(
        list, count_text, PropertyWindowList::kItemBytes,
        PropertyWindowList::kItemVtable);
    runtime.SetItem(list, count_item, 1u, row);
}

}  // namespace

void PropertyWindowList::Populate(
    PropertyWindowState& state, std::uint32_t category,
    bool preserve_scroll, IPropertyWindowListRuntime& runtime) {
    const auto list = state.Read(88u);
    const auto scroll = runtime.ScrollPosition(list);
    const auto selected = runtime.FirstSelectedItem(list);
    state.Write(108u, category);
    runtime.InitializeScratch();
    try {
        const auto selected_row = selected == 0u
            ? 0u : runtime.SelectedRow(list, selected);
        runtime.SetItemPanel(state, true);
        runtime.ResetList(list);
        std::uint32_t row = 0u;
        if (category == 5u) {
            const auto ids = runtime.RecentItemIds();
            try {
                for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
                    const auto record = runtime.RecentItem(*it);
                    if (!record) continue;
                    AppendItem(list, row++, *record, runtime);
                }
            } catch (...) {
                runtime.ReleaseRecentItems();
                throw;
            }
            runtime.ReleaseRecentItems();
        } else {
            const auto featured = runtime.FeaturedItem(category);
            if (featured) AppendItem(list, row++, *featured, runtime);
            const auto count = runtime.CategoryItemCount(category);
            for (std::uint32_t index = 0u; index < count; ++index) {
                const auto record = runtime.CategoryItem(category, index);
                if (!record || (featured &&
                                record->address == featured->address)) continue;
                AppendItem(list, row++, *record, runtime);
            }
        }

        const auto image = category < 5u
            ? kCategoryImages[category] : kCategoryImages[5];
        runtime.SetImage(state.Read(92u), "zi0", image);
        const auto rows = runtime.RowCount(list);
        if (preserve_scroll && rows > selected_row) {
            runtime.SetHoverRow(list, selected_row);
        } else if (rows != 0u) {
            runtime.SetHoverRow(list, 0u);
        }
        if (preserve_scroll) runtime.SetScrollPosition(list, scroll);
        for (std::uint32_t offset = 64u; offset <= 84u; offset += 4u) {
            runtime.SetSelected(state.Read(offset), false);
        }
        const auto selected_button = static_cast<std::int32_t>(category) < 0
            ? state.Read(84u)
            : state.Read(64u + category * 4u);
        runtime.SetSelected(selected_button, true);
        if (rows == 0u) runtime.ClearDetails(state);
    } catch (...) {
        runtime.CleanupScratch();
        throw;
    }
    runtime.CleanupScratch();
}

}  // namespace pal4
