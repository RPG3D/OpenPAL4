#pragma once
#include <cstdint>
namespace pal4 {
class ISmithWindowPanelInputRuntime {
public:
    virtual ~ISmithWindowPanelInputRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint8_t ReadByte(std::uint32_t)=0;
    virtual void RemoveActivity(std::uint32_t)=0;
    virtual bool HasHover(std::uint32_t)=0;
    virtual std::uint32_t HoverRow(std::uint32_t)=0;
    virtual std::uint32_t RowCount(std::uint32_t)=0;
    virtual void MoveHover(std::uint32_t,bool down)=0;
    virtual void SetHoverRow(std::uint32_t,std::uint32_t)=0;
    virtual void UpdateDisplay(std::uint32_t)=0;
    virtual void ConfirmTrade(std::uint32_t)=0;
    virtual void HideSelection(std::uint32_t)=0;
    virtual void TradeSelected(std::uint32_t)=0;
    virtual void SetHighlighted(std::uint32_t,bool)=0;
    virtual void ClearHighlight(std::uint32_t)=0;
    virtual void MouseAtCenter(std::uint32_t)=0;
};
class SmithWindowPanelInput final {
public:
    // Original49E7B0 /49E910; key is original event word+10, not a Win32 VK.
    static std::uint8_t Prescription(std::uint32_t member,std::uint32_t key,ISmithWindowPanelInputRuntime&);
    static std::uint8_t Equipment(std::uint32_t member,std::uint32_t key,ISmithWindowPanelInputRuntime&);
};
}
