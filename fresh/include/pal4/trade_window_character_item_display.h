#pragma once

// Keep the game interface name independent of Win32 ANSI/Unicode aliases.
#ifdef SetWindowText
#undef SetWindowText
#endif

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ITradeWindowCharacterItemDisplayRuntime {
public:
    virtual ~ITradeWindowCharacterItemDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    virtual void SetGameObjectDisplay(std::uint32_t frame,
                                      std::string_view name,
                                      std::string_view texture,
                                      std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTextBuffer() = 0;
};

class TradeWindowCharacterItemDisplay final {
public:
    // IDA 0x496200.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, std::uint32_t item,
        ITradeWindowCharacterItemDisplayRuntime& runtime);
    // IDA 0x496580; receives the record directly.
    [[nodiscard]] static std::uint32_t UpdateRecord(
        std::uint32_t frame, std::uint32_t record,
        ITradeWindowCharacterItemDisplayRuntime& runtime);
};

}  // namespace pal4
