#pragma once

// Keep the game interface name independent of Win32 ANSI/Unicode aliases.
#ifdef SetWindowText
#undef SetWindowText
#endif

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ITradeWindowPassiveItemDisplayRuntime {
public:
    virtual ~ITradeWindowPassiveItemDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameDbItem(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string DescribeGameDbItem(
        std::uint32_t item) = 0;
    virtual void CleanupDescription() = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    virtual void SetGameObjectDisplay(std::uint32_t frame,
                                      std::string_view name,
                                      std::string_view texture,
                                      std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTextBuffer() = 0;
};

class TradeWindowPassiveItemDisplay final {
public:
    // IDA 0x493910.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, std::uint32_t item,
        ITradeWindowPassiveItemDisplayRuntime& runtime);
};

}  // namespace pal4
