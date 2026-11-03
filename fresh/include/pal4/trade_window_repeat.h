#pragma once
#include <cstdint>
namespace pal4 {
// Quantity press/release and repeat timing, independent of transactions/effects.
class ITradeWindowPressRuntime {
public:
    virtual ~ITradeWindowPressRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t FrameManager()=0;
    virtual void RemoveObjectByIdentifier(std::uint32_t collection,std::uint32_t name)=0;
    virtual void RemoveUiElementByIdentifier(std::uint32_t collection,std::uint32_t name)=0;
    virtual void AddEventHandler(std::uint32_t collection,std::uint32_t name,std::uint32_t delay,
        std::uint32_t frame,std::uint32_t method,std::uint32_t row,bool decrement)=0;
};
class ITradeWindowQuantityRuntime {
public:
    virtual ~ITradeWindowQuantityRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t FrameManager()=0;
    virtual void RemoveObjectByIdentifier(std::uint32_t collection,std::uint32_t name)=0;
    virtual void AddTimerHandler(std::uint32_t collection,std::uint32_t name,std::uint32_t delay,
        std::uint32_t frame,std::uint32_t method,std::int32_t row,std::uint8_t decrement)=0;
    virtual std::uint32_t GridItem(std::uint32_t list,std::int32_t row,std::int32_t column)=0;
    virtual std::int32_t SpinnerValue(std::uint32_t item)=0;
    virtual void SetSpinnerValue(std::uint32_t item,std::int32_t value)=0;
    virtual void SetHoverRowIndex(std::uint32_t list,std::int32_t row)=0;
};
}
