#include "pal4/trade_window_item_display.h"

namespace pal4 {
namespace {

constexpr std::string_view kDescriptionPrefix =
    "<br/><colour red=0 green=0 blue=255 alpha=255>(";
constexpr std::string_view kDescriptionSuffix = ") </colour>";

void SetText(const std::uint32_t frame, const std::uint32_t offset,
             const std::string_view text,
             ITradeWindowItemDisplayRuntime& runtime) {
    runtime.SetWindowText(runtime.ReadWord(frame + offset), text);
}

void UpdatePlayerSlots(const std::uint32_t frame,
                       const std::uint32_t item,
                       ITradeWindowItemDisplayRuntime& runtime) {
    std::uint32_t visible = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.PlayerByIndex(index);
        if (player != 0u && runtime.ReadWord(player + 0xb08u) != 0u) {
            runtime.UpdatePlayerSlot(frame + 0x54u + visible * 0x20u,
                                     index, item);
            ++visible;
        }
    }
    for (; visible < 4u; ++visible)
        runtime.HidePlayerSlot(frame + 0x54u + visible * 0x20u);
}

}  // namespace

std::uint32_t TradeWindowItemDisplay::GetItemDataThunk(
    const std::uint32_t frame, const std::uint32_t item,
    ITradeWindowItemDisplayRuntime& runtime) {
    return Update(frame, item, runtime);
}

std::uint32_t TradeWindowItemDisplay::Update(
    const std::uint32_t frame, const std::uint32_t item,
    ITradeWindowItemDisplayRuntime& runtime) {
    UpdatePlayerSlots(frame, item, runtime);
    if (item == 0u) {
        SetText(frame, 0x2cu, "", runtime);
        SetText(frame, 0x30u, "", runtime);
        const auto money = static_cast<std::int32_t>(
            runtime.ReadWord(runtime.InventoryBase() + 0x134u));
        SetText(frame, 0x34u,
                runtime.ConvertToUtf8(std::to_string(money)), runtime);
        SetText(frame, 0x38u, "", runtime);
        runtime.SetGameObjectDisplay(frame, std::nullopt, 0u,
                                     std::nullopt);
        return runtime.CleanupTextBuffers();
    }

    SetText(frame, 0x2cu,
            runtime.ConvertToUtf8(
                runtime.ReadText(runtime.ReadWord(item + 0xacu))), runtime);
    auto description = runtime.BuildDescription(item);
    const auto extra = runtime.ReadText(runtime.ReadWord(item + 0xbcu));
    if (!extra.empty()) {
        description.append(kDescriptionPrefix);
        description.append(extra);
        description.append(kDescriptionSuffix);
    }
    SetText(frame, 0x30u, runtime.ConvertToUtf8(description), runtime);
    const auto money = static_cast<std::int32_t>(
        runtime.ReadWord(runtime.InventoryBase() + 0x134u));
    SetText(frame, 0x34u,
            runtime.ConvertToUtf8(std::to_string(money)), runtime);
    auto price = static_cast<std::int32_t>(
        runtime.ReadWord(item + 0x178u));
    if (runtime.ReadByte(frame + 0xf8u) == 0u) price /= 4;
    SetText(frame, 0x38u,
            runtime.ConvertToUtf8(std::to_string(price)), runtime);

    const auto category = runtime.ReadWord(item + 0x94u);
    if (category <= 6u) {
        const auto texture = runtime.ReadWord(item + 0xecu);
        const auto name = runtime.ReadText(runtime.ReadWord(item + 0xdcu));
        runtime.SetGameObjectDisplay(frame, name, texture,
                                     category <= 2u ? "PALWeapon\\" :
                                                      "PALObject\\");
    }
    return runtime.CleanupTextBuffers();
}

}  // namespace pal4
