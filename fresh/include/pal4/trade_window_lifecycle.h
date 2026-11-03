#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#ifdef SetWindowText
#undef SetWindowText
#endif
namespace pal4 {
// Visibility/close services shared by the native page and legacy game adapter.
// No layout construction, private text storage or transaction services required.
class ITradeWindowLifecycleRuntime {
public:
    virtual ~ITradeWindowLifecycleRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t FrameManager()=0;
    virtual void SetParent(std::uint32_t,bool,std::uint32_t)=0;
    virtual std::pair<std::uint32_t,std::uint32_t> CreateEffect(std::uint32_t,std::string_view,const std::array<float,3>&,std::uint32_t,bool)=0;
    virtual void ResetTemporaryEffect(std::uint32_t,std::uint32_t)=0;
    virtual void SetEffectProperties(std::uint32_t,const std::array<float,3>&,bool)=0;
    virtual void MarkEffectForUpdate(std::uint32_t)=0;
    virtual void AddAndProcessScript()=0;
    virtual std::uint32_t CreateGameObject(std::uint32_t,std::string_view,std::string_view,bool)=0;
    virtual void SetGameObjectScale(std::uint32_t,const std::array<float,3>&)=0;
    virtual void SetGameObjectPosition(std::uint32_t,const std::array<float,3>&)=0;
    virtual void CleanupScripts()=0;
    virtual void SetGameObjectDisplay(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)=0;
    virtual void RemoveGameObject(std::uint32_t,std::uint32_t)=0;
    virtual void ResetList(std::uint32_t)=0;
    virtual void SetWindowText(std::uint32_t,std::string_view)=0;
    virtual void HandleEffectEvent(std::uint32_t,std::uint32_t)=0;
    virtual void RemoveActiveFrame(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t TradeSystem()=0;
    virtual void ResetTradeSystem(std::uint32_t)=0;
    virtual void ProcessUiEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)=0;
};
}
