#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiDataRuntime;

class IShopUiActionsRuntime {
public:
    virtual ~IShopUiActionsRuntime() = default;
    virtual void GetData(std::uint32_t frame, std::uint32_t category,
                         std::uint32_t buy) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void CallVisibilitySlot(std::uint32_t function,
                                    std::uint32_t frame,
                                    std::uint32_t visible,
                                    std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual std::uint32_t SetTradeState(
        std::uint32_t trade_system) = 0;
};

class ShopUiActions final {
public:
    [[nodiscard]] static std::uint8_t OnBuyEquip(
        std::uint32_t frame, std::uint32_t event,
        IShopUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnBuyEquip(
        std::uint32_t frame, std::uint32_t event,
        IShopUiDataRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSellEquip(
        std::uint32_t frame, std::uint32_t event,
        IShopUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSellEquip(
        std::uint32_t frame, std::uint32_t event,
        IShopUiDataRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnBuyProperty(
        std::uint32_t frame, std::uint32_t event,
        IShopUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnBuyProperty(
        std::uint32_t frame, std::uint32_t event,
        IShopUiDataRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSellProperty(
        std::uint32_t frame, std::uint32_t event,
        IShopUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSellProperty(
        std::uint32_t frame, std::uint32_t event,
        IShopUiDataRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnBuyDescription(
        std::uint32_t frame, std::uint32_t event,
        IShopUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnBuyDescription(
        std::uint32_t frame, std::uint32_t event,
        IShopUiDataRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnClose(
        std::uint32_t frame, std::uint32_t event,
        IShopUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint32_t Close(
        std::uint32_t frame, IShopUiActionsRuntime& runtime);
};

}  // namespace pal4
