#pragma once

// Keep the game interface name independent of Win32 ANSI/Unicode aliases.
#ifdef SetWindowText
#undef SetWindowText
#endif

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class ITradeWindowItemDisplayRuntime {
public:
    virtual ~ITradeWindowItemDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t index) = 0;
    virtual void UpdatePlayerSlot(std::uint32_t slot,
                                  std::uint32_t player_index,
                                  std::uint32_t item) = 0;
    virtual void HidePlayerSlot(std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::string ReadText(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::string BuildDescription(
        std::uint32_t item) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    virtual void SetGameObjectDisplay(
        std::uint32_t frame, std::optional<std::string_view> name,
        std::uint32_t texture,
        std::optional<std::string_view> path) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTextBuffers() = 0;
};

class TradeWindowItemDisplay final {
public:
    // IDA 0x490000/0x490010.
    [[nodiscard]] static std::uint32_t GetItemDataThunk(
        std::uint32_t frame, std::uint32_t item,
        ITradeWindowItemDisplayRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, std::uint32_t item,
        ITradeWindowItemDisplayRuntime& runtime);
};

}  // namespace pal4
