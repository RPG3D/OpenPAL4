#include "pal4/equipment_item_details.h"

#include <cstdint>
#include <string>

namespace pal4 {
namespace {

constexpr EquipmentImageColour kUnavailableColour{
    0.57647061f, 0.72549021f, 0.87058824f, 1.0f};
constexpr EquipmentImageColour kAvailableColour{1.0f, 1.0f, 1.0f, 1.0f};

std::string TextOrEmpty(std::uint32_t address,
                        IEquipmentItemDetailsRuntime& runtime) {
    return address == 0u ? std::string{} : runtime.ReadText(address);
}

}  // namespace

std::string EquipmentItemDetails::FormatStat(
    std::int32_t original, std::int32_t projected) {
    const auto change = static_cast<std::int64_t>(projected) - original;
    if (change == 0) return " " + std::to_string(original);
    const auto value = "<font name=systemBold> " +
        std::to_string(original) + "</font><font name=systemBold>";
    if (change > 0)
        return value + "<colour red=20 green=84 blue=208 alpha=255>+" +
               std::to_string(change) + "</colour></font>";
    return value + "<colour red=204 green=48 blue=0 alpha=255>-" +
           std::to_string(-change) + "</colour></font>";
}

void EquipmentItemDetails::Update(
    EquipmentWindowState& state, std::uint32_t item,
    IEquipmentItemDetailsRuntime& runtime) {
    const auto character = state.Read(0xf4u);
    if (character == 4u) {
        for (std::uint32_t index = 0u; index < 10u; ++index)
            runtime.SetText(state.Read(0x70u + index * 4u), "");
        return;
    }

    if (item != 0u) {
        runtime.SetText(state.Read(0x68u), runtime.ConvertToUtf8(
            TextOrEmpty(runtime.ReadWord(item + 0xacu), runtime)));
        auto description = runtime.RenderItemAttributes(item);
        const auto extra = TextOrEmpty(runtime.ReadWord(item + 0xbcu), runtime);
        if (!extra.empty()) {
            description += "<br/><colour red=0 green=0 blue=0 alpha=255>(";
            description += extra;
            description += ") </colour>";
        }
        runtime.SetText(state.Read(0x6cu),
                        runtime.ConvertToUtf8(description));
    } else {
        runtime.SetText(state.Read(0x68u), "");
        runtime.SetText(state.Read(0x6cu), "");
    }

    const auto player = runtime.Player(character);
    if (player == 0u) return;
    const auto required = item != 0u && character < 4u ?
        static_cast<std::int32_t>(runtime.ReadWord(
            item + 0x168u + character * 4u)) : 0;
    const auto level = item != 0u ? runtime.PlayerLevel(player, 4u) : 0;
    const bool can_equip = item != 0u && character < 4u &&
                           required > 0 && level >= required;
    runtime.SetImageColour(state.Read(0xe0u + character * 4u),
                           item != 0u && !can_equip ?
                               kUnavailableColour : kAvailableColour);

    runtime.RefreshPlayer(player);
    const auto original = runtime.ReadStats(player);
    runtime.InitializeProjectedStats();
    auto projected = original;
    if (can_equip) {
        const auto slot = runtime.EquipmentSlot(player, item);
        const bool valid_slot = slot <= 5u;
        const auto address = player + 0x920u + slot * 4u;
        const auto saved = valid_slot ? runtime.ReadWord(address) : slot;
        if (valid_slot)
            runtime.WriteWord(address, runtime.ReadWord(item + 0x190u));
        try {
            runtime.RefreshPlayer(player);
            projected = runtime.ReadStats(player);
        } catch (...) {
            if (valid_slot) runtime.WriteWord(address, saved);
            runtime.RefreshPlayer(player);
            throw;
        }
        if (valid_slot) runtime.WriteWord(address, saved);
        runtime.RefreshPlayer(player);
    }
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.SetText(state.Read(0x70u + index * 4u),
                        FormatStat(original[index + 4u],
                                   projected[index + 4u]));
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.SetText(state.Read(0x84u + index * 4u),
                        FormatStat(original[index + 31u],
                                   projected[index + 31u]));
}

std::uint32_t EquipmentItemDetails::Clear(
    EquipmentWindowState& state,
    IEquipmentItemDetailsRuntime& runtime) {
    runtime.SetText(state.Read(0x68u), "");
    runtime.SetText(state.Read(0x6cu), "");
    for (std::uint32_t index = 0u; index < 10u; ++index)
        runtime.SetText(state.Read(0x70u + 4u * index), "");
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.SetText(state.Read(0x98u + 4u * index), "");
    runtime.SetText(state.Read(0xb4u), "");
    return runtime.RemovePreviewObject(state.Read(0xf8u));
}

}  // namespace pal4
