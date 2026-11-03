#include "pal4/trade_window_passive_item_display.h"

#include <array>

namespace pal4 {
namespace {

void SetText(const std::uint32_t frame, const std::uint32_t offset,
             const std::string_view text,
             ITradeWindowPassiveItemDisplayRuntime& runtime) {
    runtime.SetWindowText(runtime.ReadWord(frame + offset), text);
}

std::string SignedDecimal(const std::uint32_t value,
                          ITradeWindowPassiveItemDisplayRuntime& runtime) {
    return runtime.ConvertToUtf8(
        std::to_string(static_cast<std::int32_t>(value)));
}

}  // namespace

std::uint32_t TradeWindowPassiveItemDisplay::Update(
    const std::uint32_t frame, const std::uint32_t item,
    ITradeWindowPassiveItemDisplayRuntime& runtime) {
    if (item == 0u) {
        SetText(frame, 0x2cu, "", runtime);
        SetText(frame, 0x30u, "", runtime);
        SetText(frame, 0x34u,
                SignedDecimal(runtime.ReadWord(runtime.InventoryBase() +
                                               0x134u), runtime), runtime);
        SetText(frame, 0x38u, "", runtime);
        return runtime.CleanupTextBuffer();
    }

    SetText(frame, 0x2cu,
            runtime.ConvertToUtf8(
                runtime.ReadText(runtime.ReadWord(item + 0x18u))), runtime);

    const auto category = runtime.ReadWord(item + 0x64u);
    if (category == 0u) {
        const auto game_db_item = runtime.FindGameDbItem(
            runtime.ReadWord(item + 0x68u));
        const auto description = runtime.DescribeGameDbItem(game_db_item);
        try {
            SetText(frame, 0x30u, runtime.ConvertToUtf8(description), runtime);
        } catch (...) {
            runtime.CleanupDescription();
            throw;
        }
        runtime.CleanupDescription();
    } else {
        SetText(frame, 0x30u,
                runtime.ConvertToUtf8(
                    runtime.ReadText(runtime.ReadWord(item + 0x28u))),
                runtime);
    }

    SetText(frame, 0x34u,
            SignedDecimal(runtime.ReadWord(runtime.InventoryBase() +
                                           0x134u), runtime), runtime);
    SetText(frame, 0x38u,
            SignedDecimal(runtime.ReadWord(item + 0x78u), runtime), runtime);

    constexpr std::array<std::string_view, 3> textures{
        "ZZA1", "ZZA2", "ZZA3"};
    if (category < textures.size())
        runtime.SetGameObjectDisplay(frame, "zza", textures[category],
                                     "PALObject\\");
    return runtime.CleanupTextBuffer();
}

}  // namespace pal4
