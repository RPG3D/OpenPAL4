#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 7> kCategoryImages{
    "jian", "shuangjian", "qing", "toubufanju",
    "shengtifangju", "zhubufangju", "peidaia"};

}  // namespace

void SmithWindow::RefreshCollectedEquipmentList(
    const std::uint32_t object,
    ISmithWindowCollectedListRuntime& runtime) {
    const auto list = runtime.ReadWord(object + 0x40u);
    runtime.SetSortDirection(list, 0u);
    runtime.ResetList(list);

    const auto begin = runtime.ReadWord(object + 0xbcu);
    const auto end = runtime.ReadWord(object + 0xc0u);
    if (begin == end)
        throw std::logic_error("collected equipment list is empty");

    std::uint32_t last_record = 0u;
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        runtime.AddRow(list, 0u);
        const auto id = runtime.ReadWord(cursor);
        const auto record = runtime.FindInventoryItem(id, 7u);
        last_record = record;

        const auto name = runtime.Utf8Text(
            runtime.ReadWord(record + 0x9cu));
        const auto enabled = runtime.CheckPlayerCondition(record);
        const auto name_item = runtime.CreateNameItem(list, name, enabled);
        if (name_item == 0u) throw std::bad_alloc();
        runtime.WriteWord(name_item + 0x138u,
                          runtime.ReadWord(record + 0x190u));
        runtime.SetItem(list, name_item, 0u,
                        runtime.RowCount(list) - 1u);

        const auto quantity = SmithWindowItemQuantity::FirstPositive(
            record, runtime);
        const auto number_item = runtime.CreateNumberItem(
            list, std::to_string(quantity));
        runtime.SetItem(list, number_item, 1u,
                        runtime.RowCount(list) - 1u);
    }

    const auto category = runtime.ReadWord(last_record + 0x94u);
    const auto category_name = runtime.CategoryName(category);
    const auto image_window = runtime.ReadWord(object + 0x6cu);
    runtime.SetText(image_window, category_name
        ? runtime.Utf8Buffer(*category_name) : std::string{});
    if (category < kCategoryImages.size())
        runtime.SetImage(image_window, "zi0", kCategoryImages[category]);

    if (runtime.RowCount(list) != 0u) runtime.SetHoverRow(list, 0u);
    const auto hover_item = runtime.HoverItem(list);
    const auto selected = hover_item == 0u ? 0u
        : runtime.FindInventoryItem(
              runtime.ReadWord(hover_item + 0x138u), 7u);
    UpdateEquipmentItemName(object, selected, runtime);
    UpdateEquipmentItemDescription(object, selected, runtime);
    runtime.Call49F080(object);
    runtime.Call49EF80(object);
    runtime.SetVisible(runtime.ReadWord(object + 0x58u), false);
    runtime.SetVisible(runtime.ReadWord(object + 0x5cu), false);
}

}  // namespace pal4
