#include "pal4/trade_window_character_item_display.h"

namespace pal4 {
namespace {

void SetText(const std::uint32_t frame, const std::uint32_t offset,
             const std::string_view text,
             ITradeWindowCharacterItemDisplayRuntime& runtime) {
    runtime.SetWindowText(runtime.ReadWord(frame + offset), text);
}

std::string ItemText(const std::uint32_t record,
                     const std::uint32_t offset,
                     ITradeWindowCharacterItemDisplayRuntime& runtime) {
    return runtime.ConvertToUtf8(
        runtime.ReadText(runtime.ReadWord(record + offset)));
}

void SetBalance(const std::uint32_t frame,
                ITradeWindowCharacterItemDisplayRuntime& runtime) {
    SetText(frame, 0x34u,
            runtime.ConvertToUtf8(std::to_string(
                static_cast<std::int32_t>(runtime.ReadWord(
                    runtime.InventoryBase() + 0x134u)))), runtime);
}

std::uint32_t UpdateRecordImpl(
    const std::uint32_t frame, const std::uint32_t record,
    const bool balance_first,
    ITradeWindowCharacterItemDisplayRuntime& runtime) {
    if (record == 0u) {
        SetText(frame, 0x2cu, "", runtime);
        SetBalance(frame, runtime);
        SetText(frame, 0x30u, "", runtime);
        runtime.SetGameObjectDisplay(frame, "", "", "");
        return runtime.CleanupTextBuffer();
    }

    SetText(frame, 0x2cu, ItemText(record, 0x24u, runtime), runtime);
    if (balance_first) SetBalance(frame, runtime);
    SetText(frame, 0x30u, ItemText(record, 0x34u, runtime), runtime);
    if (!balance_first) SetBalance(frame, runtime);
    runtime.SetGameObjectDisplay(
        frame, runtime.ReadText(runtime.ReadWord(record + 0x54u)),
        runtime.ReadText(runtime.ReadWord(record + 0x64u)),
        "PALObject\\");
    return runtime.CleanupTextBuffer();
}

}  // namespace

std::uint32_t TradeWindowCharacterItemDisplay::Update(
    const std::uint32_t frame, const std::uint32_t item,
    ITradeWindowCharacterItemDisplayRuntime& runtime) {
    return UpdateRecordImpl(frame, item == 0u ? 0u : runtime.ReadWord(item),
                            false, runtime);
}

std::uint32_t TradeWindowCharacterItemDisplay::UpdateRecord(
    const std::uint32_t frame, const std::uint32_t record,
    ITradeWindowCharacterItemDisplayRuntime& runtime) {
    return UpdateRecordImpl(frame, record, true, runtime);
}

}  // namespace pal4
