#include "pal4/trade_window_passive_pages.h"

#include <array>
#include <new>
#include <string>

namespace pal4 {
namespace {

struct Column {
    std::uint32_t id;
    float width;
    std::string_view label;
};

constexpr std::array<Column, 4> kObjectColumns{{
    {0u, 0.44999999f, "mingcheng"},
    {1u, 0.15000001f, "leibie"},
    {2u, 0.15000001f, "dengji"},
    {3u, 0.15000001f, "jiage"},
}};
constexpr std::array<Column, 3> kMaterialColumns{{
    {0u, 0.40000001f, "mingcheng"},
    {1u, 0.30000001f, "xuyaoqianli"},
    {2u, 0.2f, "jiage"},
}};
constexpr std::array<Column, 3> kSpiritColumns{{
    {0u, 0.40000001f, "mingcheng"},
    {1u, 0.30000001f, "xuyaolingxu"},
    {2u, 0.2f, "jiage"},
}};
constexpr std::array<std::string_view, 3> kPageImages{
    "rongzhutupu", "duanyetupu", "zhulingtupu"};

template <std::size_t N>
void AddColumns(const std::uint32_t list,
                const std::array<Column, N>& columns,
                ITradeWindowPassivePageRuntime& runtime) {
    for (const auto& column : columns) {
        runtime.AddColumn(list, column.id, column.width);
    }
    for (const auto& column : columns) {
        runtime.SetColumnLabelImage(list, column.id, "zi0", column.label);
    }
}

void AddCell(const std::uint32_t list, const std::uint32_t column,
             const std::uint32_t id, const std::uint8_t disabled,
             const std::string_view text,
             ITradeWindowPassivePageRuntime& runtime) {
    const auto item = runtime.CreateItem(0x210u, list, text);
    if (item == 0u) throw std::bad_alloc();
    runtime.WriteWord(item + 0x138u, id);
    runtime.WriteByte(item + 0x1a5u, disabled);
    runtime.SetItem(list, item, column, runtime.RowCount(list) - 1u);
}

std::int32_t FirstNonzeroQuantity(
    const std::uint32_t item, ITradeWindowPassivePageRuntime& runtime) {
    for (std::uint32_t i = 0; i < 4u; ++i) {
        const auto quantity = runtime.ReadWord(item + 0x168u + 4u * i);
        if (quantity != 0u) return static_cast<std::int32_t>(quantity);
    }
    return 0;
}

void AddRecord(const std::uint32_t list, const std::uint32_t page,
               const std::uint32_t record,
               ITradeWindowPassivePageRuntime& runtime) {
    std::uint32_t game_db_item = 0u;
    if (page == 0u) {
        game_db_item = runtime.FindGameDbItem(
            runtime.ReadWord(record + 0x68u));
        if (game_db_item == 0u) return;
    }

    const auto id = runtime.ReadWord(record);
    const auto inventory_result = runtime.InventorySearch(id);
    const auto price = static_cast<std::int32_t>(
        runtime.ReadWord(record + 0x78u));
    const auto money = static_cast<std::int32_t>(
        runtime.ReadWord(runtime.InventoryBase() + 0x134u));
    const auto disabled = static_cast<std::uint8_t>(
        (inventory_result & 0xffu) | (price > money ? 1u : 0u));
    runtime.AddRow(list);
    AddCell(list, 0u, id, disabled,
            runtime.Utf8Name(runtime.ReadWord(record + 8u)), runtime);

    if (page == 0u) {
        const auto effect = runtime.FindEffect(game_db_item);
        if (effect != 0u) {
            AddCell(list, 1u, id, disabled,
                    runtime.Utf8Name(runtime.ReadWord(effect + 4u)), runtime);
        }
        AddCell(list, 2u, id, disabled,
                std::to_string(FirstNonzeroQuantity(game_db_item, runtime)),
                runtime);
        AddCell(list, 3u, id, disabled, std::to_string(price), runtime);
    } else {
        const auto requirement_offset = page == 1u ? 0x6cu : 0x70u;
        const auto requirement = static_cast<std::int32_t>(
            runtime.ReadWord(record + requirement_offset));
        AddCell(list, 1u, id, disabled,
                std::to_string(requirement), runtime);
        AddCell(list, 2u, id, disabled, std::to_string(price), runtime);
    }
}

}  // namespace

void TradeWindowPassivePages::Update(
    const std::uint32_t frame, const std::uint32_t page,
    ITradeWindowPassivePageRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    if (!runtime.IsTradeList(list) ||
        runtime.ReadByte(frame + 0xf8u) == 0u) return;

    runtime.WriteWord(frame + 0xf4u, page);
    runtime.SetImage(runtime.ReadWord(frame + 0xe4u),
                     runtime.ReadWord(frame + 0x190u + 4u * page));
    runtime.ResetList(list);
    const auto old_columns = runtime.ColumnCount(list);
    for (std::uint32_t i = 0u; i < old_columns; ++i)
        runtime.RemoveColumn(list, 0u);

    if (page < kPageImages.size()) {
        if (page == 0u) AddColumns(list, kObjectColumns, runtime);
        else if (page == 1u) AddColumns(list, kMaterialColumns, runtime);
        else AddColumns(list, kSpiritColumns, runtime);

        const auto begin = runtime.ReadWord(frame + 0x154u);
        const auto end = runtime.ReadWord(frame + 0x158u);
        if (begin != 0u) {
            for (auto address = begin; address < end; address += 4u) {
                const auto record = runtime.ReadWord(address);
                if (runtime.ReadWord(record + 0x64u) == page)
                    AddRecord(list, page, record, runtime);
            }
        }
    }

    if (runtime.RowCount(list) != 0u)
        runtime.SetHoverRowIndex(list, 0);
    else
        runtime.ClearItemDisplay(frame);
    if (page < kPageImages.size())
        runtime.SetNamedImage(runtime.ReadWord(frame + 0x50u),
                              "zi0", kPageImages[page]);
}

}  // namespace pal4
