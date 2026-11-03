#include "pal4/trade_window_shop_pages.h"

#include <array>
#include <new>
#include <string>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 7> kCategoryImages{
    "jian", "shuangjian", "qing", "toubufanju",
    "shengtifangju", "zhubufangju", "peidaia"};

std::uint32_t CreateItem(const std::uint32_t list,
                         const std::uint32_t bytes,
                         const std::string_view text,
                         const bool special,
                         const bool condition,
                         ITradeWindowShopPageRuntime& runtime) {
    const auto item = runtime.CreateItem(bytes, list, text, special,
                                         condition);
    if (item == 0u) throw std::bad_alloc();
    return item;
}

std::int32_t FirstPositiveQuantity(const std::uint32_t record,
                                   ITradeWindowShopPageRuntime& runtime) {
    std::int32_t value = 0;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        value = static_cast<std::int32_t>(
            runtime.ReadWord(record + 0x168u + 4u * i));
        if (value > 0) break;
    }
    return value;
}

void SetCell(const std::uint32_t list, const std::uint32_t item,
             const std::uint32_t column, const std::uint32_t id,
             const bool disabled, const bool flag_before_insert,
             ITradeWindowShopPageRuntime& runtime) {
    runtime.WriteWord(item + 0x138u, id);
    if (flag_before_insert)
        runtime.WriteByte(item + 0x1a5u, disabled ? 1u : 0u);
    const auto row = runtime.RowCount(list) - 1u;
    runtime.SetItem(list, item, column, row);
    if (!flag_before_insert)
        runtime.WriteByte(item + 0x1a5u, disabled ? 1u : 0u);
}

void InitializeList(const std::uint32_t list,
                    ITradeWindowShopPageRuntime& runtime) {
    runtime.ResetList(list);
    const auto columns = runtime.ColumnCount(list);
    for (std::uint32_t i = 0u; i < columns; ++i)
        runtime.RemoveColumn(list, 0u);
    constexpr std::array<std::uint32_t, 3> ids{0u, 1u, 3u};
    constexpr std::array<float, 3> widths{0.44999999f, 0.25f, 0.2f};
    constexpr std::array<std::string_view, 3> labels{
        "mingcheng", "dengji", "jiage"};
    for (std::size_t i = 0u; i < ids.size(); ++i) {
        runtime.AddColumn(list, ids[i], widths[i]);
        runtime.SetColumnLabelImage(list, ids[i], "zi0", labels[i]);
    }
}

void UpdateCategoryImage(const std::uint32_t frame,
                         const std::uint32_t category,
                         ITradeWindowShopPageRuntime& runtime) {
    runtime.SetNamedImage(runtime.ReadWord(frame + 0x50u), "zi0",
        category < kCategoryImages.size()
            ? kCategoryImages[category] : "xinzenga");
}

void FinishPage(const std::uint32_t frame, const std::uint32_t list,
                const std::uint32_t category,
                ITradeWindowShopPageRuntime& runtime) {
    if (runtime.RowCount(list) != 0u)
        runtime.SetHoverRowIndex(list, 0);
    else
        runtime.ClearItemDisplay(frame);
    UpdateCategoryImage(frame, category, runtime);
}

std::uint32_t BeginPage(const std::uint32_t frame,
                        const std::uint32_t page, const bool active,
                        ITradeWindowShopPageRuntime& runtime) {
    const auto shown = active && page == 7u ? 0u : page;
    runtime.WriteWord(frame + 0xecu, shown);
    const auto image_index = runtime.ImageIndex(page);
    if (image_index < 5u) {
        runtime.SetImage(runtime.ReadWord(frame + 0xe4u),
                         runtime.ReadWord(frame + 0x160u + 4u * image_index));
    }
    const auto list = runtime.ReadWord(frame + 0x28u);
    InitializeList(list, runtime);
    return list;
}

void AddInactiveRow(const std::uint32_t list, const std::uint32_t record,
                    ITradeWindowShopPageRuntime& runtime) {
    runtime.AddRow(list);
    const auto disabled = runtime.CheckPlayersCondition(record);
    const auto name = runtime.Utf8Name(runtime.ReadWord(record + 0x9cu));
    const auto item_id = runtime.ReadWord(record + 0x190u);
    const auto name_item = CreateItem(list, 0x214u, name, true,
                                     runtime.CheckPlayersCondition(record),
                                     runtime);
    SetCell(list, name_item, 0u, item_id, disabled, false, runtime);
    const auto quantity = FirstPositiveQuantity(record, runtime);
    const auto quantity_item = CreateItem(list, 0x210u,
        std::to_string(quantity), false, false, runtime);
    SetCell(list, quantity_item, 1u, item_id, disabled, false, runtime);
    const auto price = static_cast<std::int32_t>(
        runtime.ReadWord(record + 0x178u)) / 4;
    const auto price_item = CreateItem(list, 0x210u,
        std::to_string(price), false, false, runtime);
    SetCell(list, price_item, 3u,
            runtime.ReadWord(record + 0x90u), disabled, false, runtime);
}

void AddActiveRow(const std::uint32_t list, const std::uint32_t record,
                  ITradeWindowShopPageRuntime& runtime) {
    const auto price = static_cast<std::int32_t>(
        runtime.ReadWord(record + 0x178u));
    const auto money = static_cast<std::int32_t>(
        runtime.ReadWord(runtime.InventoryBase() + 0x134u));
    const auto disabled = price > money;
    runtime.AddRow(list);
    const auto name = runtime.Utf8Name(runtime.ReadWord(record + 0x9cu));
    const auto item_id = runtime.ReadWord(record + 0x90u);
    const auto name_item = CreateItem(list, 0x210u, name, false,
                                     false, runtime);
    SetCell(list, name_item, 0u, item_id, disabled, true, runtime);
    const auto quantity_item = CreateItem(list, 0x210u,
        std::to_string(FirstPositiveQuantity(record, runtime)),
        false, false, runtime);
    SetCell(list, quantity_item, 1u, item_id, disabled, true, runtime);
    const auto price_item = CreateItem(list, 0x210u,
        std::to_string(price), false, false, runtime);
    SetCell(list, price_item, 3u, item_id, disabled, true, runtime);
}

}  // namespace

std::uint32_t TradeWindowShopPages::UpdateInactive(
    const std::uint32_t frame, const std::uint32_t page,
    ITradeWindowShopPageRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    if (!runtime.IsTradeList(list)) return 1u;
    const auto current_list = BeginPage(frame, page, false, runtime);
    const auto count = runtime.InventorySize(page);
    for (std::uint32_t i = 0u; i < count; ++i) {
        const auto record = runtime.InventoryItem(page, i);
        if (runtime.CheckPlayersConditionTwo(record))
            AddInactiveRow(current_list, record, runtime);
    }
    FinishPage(frame, current_list, page, runtime);
    return runtime.CleanupTemporaryName();
}

std::uint32_t TradeWindowShopPages::UpdateActive(
    const std::uint32_t frame, const std::uint32_t page,
    ITradeWindowShopPageRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    if (!runtime.IsTradeList(list)) return 1u;
    const auto current_list = BeginPage(frame, page, true, runtime);
    const auto begin = runtime.ReadWord(frame + 0x134u);
    const auto end = runtime.ReadWord(frame + 0x138u);
    if (begin != 0u) {
        for (auto address = begin; address < end; address += 4u) {
            const auto record = runtime.ReadWord(address);
            if (runtime.ReadWord(record + 0x94u) ==
                runtime.ReadWord(frame + 0xecu))
                AddActiveRow(current_list, record, runtime);
        }
    }
    FinishPage(frame, current_list, page, runtime);
    return runtime.CleanupTemporaryName();
}

}  // namespace pal4
