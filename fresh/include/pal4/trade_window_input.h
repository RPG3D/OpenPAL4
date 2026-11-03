#pragma once

#include <cstdint>

namespace pal4 {

class ITradeWindowInputRuntime {
public:
    virtual ~ITradeWindowInputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    virtual void UpHoverItem(std::uint32_t list) = 0;
    virtual void DownHoverItem(std::uint32_t list) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::int32_t row) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool recurse) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::int32_t HoverRowOfItem(
        std::uint32_t list, std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t GridItem(
        std::uint32_t list, std::int32_t row,
        std::int32_t column) = 0;
    [[nodiscard]] virtual std::int32_t SpinnerValue(
        std::uint32_t item) = 0;
    virtual void SetSpinnerValue(std::uint32_t item,
                                 std::int32_t value) = 0;
    virtual void CloseTradeWindow(std::uint32_t frame) = 0;
    virtual void ShowTradePrompt(std::uint32_t frame) = 0;
};

class TradeWindowInput final {
public:
    // IDA 0x48d750/0x48d900/0x48d910/0x48d990.
    [[nodiscard]] static bool OnKeyboardInput(
        std::uint32_t input, std::uint32_t event,
        ITradeWindowInputRuntime& runtime);
    [[nodiscard]] static bool OnStatusChanged(
        std::uint32_t input, ITradeWindowInputRuntime& runtime);
    static void SetItemGlow(std::uint32_t input, bool glow,
                            ITradeWindowInputRuntime& runtime);
    [[nodiscard]] static bool OnQuantityKeyInput(
        std::uint32_t input, std::uint32_t event,
        ITradeWindowInputRuntime& runtime);
};

}  // namespace pal4
