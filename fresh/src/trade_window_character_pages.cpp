#include "pal4/trade_window_character_pages.h"

#include <array>
#include <bit>
#include <new>
#include <optional>
#include <string>

namespace pal4 {
namespace {

struct Column {
    float width;
    std::string_view label;
};

constexpr std::array<Column, 4> kColumns{{
    {0.38f, "mingcheng"},
    {0.2f, "shuliang"},
    {0.15000001f, "cunliang"},
    {0.17f, "jiage"},
}};
constexpr std::array<std::string_view, 5> kPageImages{
    "huifu", "gongjiwupin", "fuzhuwuping", "cailiao", "juqing"};

std::uint32_t AddTextCell(const std::uint32_t list,
                          const std::uint32_t column,
                          const std::uint32_t id,
                          const std::string_view text,
                          ITradeWindowCharacterPageRuntime& runtime,
                          const std::optional<std::uint8_t> disabled =
                              std::nullopt) {
    const auto item = runtime.CreateTextItem(0x210u, list, text);
    if (item == 0u) throw std::bad_alloc();
    runtime.WriteWord(item + 0x138u, id);
    if (disabled) runtime.WriteByte(item + 0x1a5u, *disabled);
    runtime.SetItem(list, item, column, runtime.RowCount(list) - 1u);
    return item;
}

void AddSpinner(const std::uint32_t list, const std::uint32_t id,
                const std::int32_t maximum,
                ITradeWindowCharacterPageRuntime& runtime,
                const std::optional<std::uint8_t> disabled =
                    std::nullopt) {
    const auto spinner = runtime.CreateSpinnerItem(0x1b4u);
    if (spinner == 0u) throw std::bad_alloc();
    runtime.WriteWord(spinner + 0x138u, id);
    if (disabled) runtime.WriteByte(spinner + 0x1a5u, *disabled);
    runtime.SetItem(list, spinner, 1u, runtime.RowCount(list) - 1u);
    runtime.SetSpinnerRange(spinner, 0, maximum);
    runtime.WriteWord(spinner + 0x134u, runtime.RowCount(list) - 1u);
    runtime.BindSpinnerState(spinner);
}

void AddInactiveRow(const std::uint32_t list, const std::uint32_t element,
                    ITradeWindowCharacterPageRuntime& runtime) {
    const auto record = runtime.ReadWord(element);
    const auto id = runtime.ReadWord(element + 4u);
    const auto available = static_cast<std::int32_t>(
        runtime.ReadWord(element + 8u));
    runtime.AddRow(list);
    AddTextCell(list, 0u, id,
                runtime.Utf8Name(runtime.ReadWord(record + 0x14u)),
                runtime);

    AddSpinner(list, id, available, runtime);

    AddTextCell(list, 2u, id, std::to_string(available), runtime);
    const auto base_price = static_cast<std::int32_t>(
        runtime.ReadWord(record + 0x90u));
    AddTextCell(list, 3u, id, std::to_string(base_price / 4), runtime);
}

void PreparePage(const std::uint32_t frame, const std::uint32_t page,
                 const std::uint32_t list, const bool active,
                 ITradeWindowCharacterPageRuntime& runtime) {
    runtime.WriteWord(frame + 0xf0u, page);
    if (std::bit_cast<std::int32_t>(page) >= 0 && page < 5u &&
        (!active || page != 0u)) {
        runtime.SetImage(runtime.ReadWord(frame + 0xe4u),
                         runtime.ReadWord(frame + 0x178u + 4u * page));
    }
    runtime.ResetList(list);
    const auto old_columns = runtime.ColumnCount(list);
    for (std::uint32_t i = 0u; i < old_columns; ++i)
        runtime.RemoveColumn(list, 0u);
    for (std::uint32_t i = 0u; i < kColumns.size(); ++i) {
        runtime.AddColumn(list, i, kColumns[i].width);
        runtime.SetColumnLabelImage(list, i, "zi0", kColumns[i].label);
    }
}

void FillInactivePage(const std::uint32_t list, const std::uint32_t page,
                      ITradeWindowCharacterPageRuntime& runtime) {
    const auto count = runtime.InventorySize(page);
    for (std::int32_t i = 0; i < count; ++i) {
        const auto element = runtime.InventoryElement(
            page, static_cast<std::uint32_t>(i));
        const auto record = runtime.ReadWord(element);
        if (runtime.ReadWord(record + 0xa0u) != 0u)
            AddInactiveRow(list, element, runtime);
    }
}

void AddActiveRow(const std::uint32_t list, const std::uint32_t page,
                  const std::uint32_t record,
                  ITradeWindowCharacterPageRuntime& runtime) {
    runtime.AddRow(list);
    const auto id = runtime.ReadWord(record);
    const auto price = static_cast<std::int32_t>(
        runtime.ReadWord(record + 0x90u));
    const auto money = static_cast<std::int32_t>(
        runtime.ReadWord(runtime.InventoryBase() + 0x134u));
    const auto disabled = static_cast<std::uint8_t>(price > money ? 1u : 0u);
    AddTextCell(list, 0u, id,
                runtime.Utf8Name(runtime.ReadWord(record + 0x14u)),
                runtime, disabled);

    const auto element = runtime.FindInventoryElement(id, page);
    const auto owned = element == 0u ? 0u : runtime.ReadWord(element + 8u);
    const auto limit = std::bit_cast<std::int32_t>(99u - owned);
    AddSpinner(list, id, limit, runtime, disabled);
    AddTextCell(list, 2u, id,
                std::to_string(std::bit_cast<std::int32_t>(owned)),
                runtime, disabled);
    AddTextCell(list, 3u, id, std::to_string(price), runtime, disabled);
}

void FillActivePage(const std::uint32_t frame, const std::uint32_t list,
                    const std::uint32_t page,
                    ITradeWindowCharacterPageRuntime& runtime) {
    const auto begin = runtime.ReadWord(frame + 0x144u);
    if (begin == 0u) return;
    const auto end = runtime.ReadWord(frame + 0x148u);
    for (auto address = begin; address < end; address += 4u) {
        const auto record = runtime.ReadWord(address);
        if (runtime.ReadWord(record + 4u) == page)
            AddActiveRow(list, page, record, runtime);
    }
}

void FinishPage(const std::uint32_t frame, const std::uint32_t page,
                const std::uint32_t list,
                ITradeWindowCharacterPageRuntime& runtime) {
    if (runtime.RowCount(list) != 0u)
        runtime.SetHoverRowIndex(list, 0);
    else
        runtime.ClearItemDisplay(frame);
    const auto image = page < kPageImages.size()
        ? kPageImages[page] : kPageImages.front();
    runtime.SetNamedImage(runtime.ReadWord(frame + 0x50u), "zi0", image);
}

std::uint32_t UpdatePage(const std::uint32_t frame,
                         const std::uint32_t page,
                         const bool active,
                         ITradeWindowCharacterPageRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    if (!runtime.IsTradeList(list)) return 1u;

    bool cleanup_started = false;
    try {
        PreparePage(frame, page, list, active, runtime);
        if (active) FillActivePage(frame, list, page, runtime);
        else FillInactivePage(list, page, runtime);
        FinishPage(frame, page, list, runtime);
        cleanup_started = true;
        return runtime.CleanupPageBuffers();
    } catch (...) {
        if (!cleanup_started) (void)runtime.CleanupPageBuffers();
        throw;
    }
}

}  // namespace

std::uint32_t TradeWindowCharacterPages::Update(
    const std::uint32_t frame, const std::uint32_t page,
    ITradeWindowCharacterPageRuntime& runtime) {
    return UpdatePage(frame, page, false, runtime);
}

std::uint32_t TradeWindowCharacterPages::UpdateActive(
    const std::uint32_t frame, const std::uint32_t page,
    ITradeWindowCharacterPageRuntime& runtime) {
    return UpdatePage(frame, page, true, runtime);
}

}  // namespace pal4
