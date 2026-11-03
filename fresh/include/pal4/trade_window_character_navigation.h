#pragma once

#include <cstdint>

namespace pal4 {

class ITradeWindowCharacterNavigationRuntime {
public:
    virtual ~ITradeWindowCharacterNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void FillActiveCharacter(std::uint32_t frame,
                                     std::uint32_t index) = 0;
    virtual void FillInactiveCharacter(std::uint32_t frame,
                                       std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t NextCharacterIndex(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PreviousCharacterIndex(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual bool HasTradeItems(std::uint32_t frame) = 0;
    [[nodiscard]] virtual bool CanExecuteTrade(std::uint32_t frame) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
};

class TradeWindowCharacterNavigation final {
public:
    // IDA 0x493f30.
    static void Update(std::uint32_t frame, std::uint32_t first_index,
                       bool forward,
                       ITradeWindowCharacterNavigationRuntime& runtime);
};

}  // namespace pal4
