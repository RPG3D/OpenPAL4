#pragma once
#include <cstdint>
namespace pal4 {
class ITradeWindowHoverRuntime {
public:
    virtual ~ITradeWindowHoverRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint8_t ReadByte(std::uint32_t)=0;
    virtual std::uint32_t GetItemFromHover(std::uint32_t)=0;
    virtual std::uint32_t GetInventoryFromHover(std::uint32_t)=0;
    virtual std::uint32_t GetPassiveFromHover(std::uint32_t)=0;
    virtual std::uint32_t GetBuyItemFromHover(std::uint32_t)=0;
    virtual std::uint32_t GetBuyInventoryFromHover(std::uint32_t)=0;
    virtual void UpdateItemDisplay(std::uint32_t,std::uint32_t)=0;
    virtual void InvokeHoverHandler(std::uint32_t,std::uint32_t,std::uint32_t)=0;
    virtual void RefreshTotalPrice(std::uint32_t)=0;
};
}
