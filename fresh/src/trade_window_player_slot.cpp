#include "pal4/trade_window_player_slot.h"
#include "pal4/trade_window_percentage_formatter.h"

#include <bit>
#include <stdexcept>

namespace pal4 {
namespace {

class PreviewGuard {
public:
    explicit PreviewGuard(ITradeWindowPlayerSlotRuntime& runtime)
        : runtime_(runtime) {}
    ~PreviewGuard() { runtime_.CleanupPreview(); }
    PreviewGuard(const PreviewGuard&) = delete;
    PreviewGuard& operator=(const PreviewGuard&) = delete;

private:
    ITradeWindowPlayerSlotRuntime& runtime_;
};

void SetSlotText(std::uint32_t slot, std::uint32_t index,
                 std::string_view text,
                 ITradeWindowPlayerSlotRuntime& runtime) {
    runtime.SetText(runtime.ReadWord(slot + index * 4u), text);
}

std::int32_t Subtract(std::int32_t left, std::int32_t right) {
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) -
        static_cast<std::uint32_t>(right));
}

std::string FormatDifference(std::int32_t value) {
    if (value == 0) return {};
    if (value > 0)
        return "<font name=systemBold><colour red=20 green=84 blue=208 alpha=255>+" +
            std::to_string(value) + "</colour></font>";
    const auto magnitude = 0u - static_cast<std::uint32_t>(value);
    return "<font name=systemBold><colour red=204 green=48 blue=0 alpha=255>-" +
        std::to_string(magnitude) + "</colour></font>";
}

}  // namespace

std::uint32_t TradeWindowPlayerSlot::Update(
    std::uint32_t slot, std::uint32_t player_index,
    std::uint32_t item, ITradeWindowPlayerSlotRuntime& runtime) {
    const auto icon = runtime.ReadWord(slot);
    runtime.WriteByte(icon + 0x6b4u, 0u);
    const auto player = runtime.PlayerByIndex(player_index);
    const auto active = player != 0u &&
        runtime.ReadWord(player + 0xb08u) != 0u;
    for (std::uint32_t index = 2u; index <= 6u; ++index)
        runtime.SetVisible(runtime.ReadWord(slot + index * 4u), active);
    runtime.SetVisible(icon, active);
    runtime.SetVisible(runtime.ReadWord(slot + 4u), active);
    runtime.SetIconImage(icon, runtime.IconImage(player_index, 2u, 0u));
    if (player == 0u)
        throw std::invalid_argument("player slot requires an existing player");

    runtime.SetText(runtime.ReadWord(slot + 0x1cu),
                    TradeWindowPercentageFormatter::Format(
                        runtime.PlayerStat(player, 4u)));

    if (item != 0u && active) {
        const auto stat = runtime.PlayerStat(player, 4u);
        const auto threshold = player_index < 4u ?
            static_cast<std::int32_t>(runtime.ReadWord(
                item + 0x168u + player_index * 4u)) : 0;
        if (player_index < 4u && threshold > 0 && stat >= threshold) {
            runtime.SetIconColour(icon, {1.0f, 1.0f, 1.0f, 1.0f});
            const auto equipment = player + 0x920u;
            const auto excluded = runtime.ClassifySlot(equipment, item);
            runtime.RefreshEquipment(player + 0x654u);
            std::array<std::int32_t, 5> previous{};
            for (std::uint32_t index = 0u; index < previous.size(); ++index)
                previous[index] = static_cast<std::int32_t>(
                    runtime.ReadWord(player + 0x7b8u + index * 4u));
            runtime.StartPreview(item);
            PreviewGuard guard(runtime);
            for (std::uint32_t index = 0u; index < 6u; ++index) {
                if (static_cast<std::int32_t>(index) == excluded) continue;
                const auto id = runtime.ReadWord(equipment + index * 4u);
                const auto record = runtime.FindInventoryItem(
                    runtime.InventoryBase() + 0x7cu, id, 7u);
                if (record != 0u) runtime.AddPreviewRecord(record);
            }
            const auto updated = runtime.FinishPreview(player);
            for (std::uint32_t index = 0u; index < updated.size(); ++index)
                SetSlotText(slot, index + 2u,
                            runtime.ConvertToUtf8(FormatDifference(
                                Subtract(updated[index], previous[index]))),
                            runtime);
        } else {
            for (std::uint32_t index = 2u; index <= 6u; ++index)
                runtime.SetVisible(runtime.ReadWord(slot + index * 4u), false);
            runtime.SetIconColour(icon,
                                  {0.57647061f, 0.72549021f,
                                   0.87058824f, 1.0f});
        }
    } else {
        for (std::uint32_t index = 2u; index <= 6u; ++index)
            SetSlotText(slot, index, "", runtime);
    }
    return runtime.CleanupTextBuffers();
}

void TradeWindowPlayerSlot::Hide(
    std::uint32_t slot, ITradeWindowPlayerSlotRuntime& runtime) {
    for (std::uint32_t index = 2u; index <= 6u; ++index)
        runtime.SetVisible(runtime.ReadWord(slot + index * 4u), false);
    runtime.SetVisible(runtime.ReadWord(slot), false);
    runtime.SetVisible(runtime.ReadWord(slot + 4u), false);
}

}  // namespace pal4
