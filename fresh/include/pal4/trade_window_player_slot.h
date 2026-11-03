#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct TradeSlotColour {
    float red;
    float green;
    float blue;
    float alpha;
};

class ITradeWindowPlayerSlotRuntime {
public:
    virtual ~ITradeWindowPlayerSlotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(std::uint32_t index) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t IconImage(
        std::uint32_t index, std::uint32_t type,
        std::uint32_t variant) = 0;
    virtual void SetIconImage(std::uint32_t window, std::uint32_t image) = 0;
    [[nodiscard]] virtual std::int32_t PlayerStat(
        std::uint32_t player, std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(std::string_view text) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void SetIconColour(std::uint32_t window,
                               TradeSlotColour colour) = 0;
    [[nodiscard]] virtual std::int32_t ClassifySlot(
        std::uint32_t equipment, std::uint32_t item) = 0;
    virtual void RefreshEquipment(std::uint32_t component) = 0;
    virtual void StartPreview(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t collection, std::uint32_t id,
        std::uint32_t category) = 0;
    virtual void AddPreviewRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::array<std::int32_t, 5> FinishPreview(
        std::uint32_t player) = 0;
    virtual void CleanupPreview() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTextBuffers() = 0;
};

class TradeWindowPlayerSlot final {
public:
    // IDA 0x4907a0.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t slot, std::uint32_t player_index,
        std::uint32_t item, ITradeWindowPlayerSlotRuntime& runtime);
    // IDA 0x491430.
    static void Hide(std::uint32_t slot,
                     ITradeWindowPlayerSlotRuntime& runtime);
};

}  // namespace pal4
