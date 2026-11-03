#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiDataRuntime {
public:
    virtual ~IShopUiDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void CallVisibilitySlot(std::uint32_t function,
                                    std::uint32_t frame,
                                    std::uint32_t visible,
                                    std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    virtual void EnsureGameDatabase(std::uint32_t trade_system) = 0;
    [[nodiscard]] virtual std::uint8_t OpenShop(
        std::uint32_t trade_system, std::uint32_t data,
        std::uint8_t buy, std::uint32_t category) = 0;
};

class ShopUiData final {
public:
    // IDA 0x486850.
    [[nodiscard]] static std::uint8_t Load(
        std::uint32_t frame, std::uint32_t category,
        std::uint8_t buy, IShopUiDataRuntime& runtime);
};

}  // namespace pal4
