#pragma once
#include "pal4/smith_state.h"
#include "pal4/smith_system_access.h"
#include <functional>
namespace pal4 {
class ISystemMenuRuntime;class GameRenderState;
// Borrow the existing native UI/camera/frame/effect runtime and the same flag
// byte used by the Smith page/toolbar. No second UI collection is created.
class NativeSmithStateRuntime final : public ISmithStateRuntime,private ISmithUiEventRuntime {
public:
    using ConfigReader=std::function<std::optional<std::uint32_t>(std::uint32_t,std::string_view)>;
    NativeSmithStateRuntime(ISystemMenuRuntime&,GameRenderState&,std::uint8_t& smith_flag,ConfigReader);
    std::uint32_t UIFrameManager()override;
    void SetUIFlag(std::uint32_t,std::uint32_t)override;
    std::uint32_t Console()override;
    void WriteWord(std::uint32_t,std::uint32_t)override;
    std::uint32_t ReadWord(std::uint32_t)override;
    void AddUIElement(std::uint32_t,std::string_view)override;
    std::uint32_t GameState()override;
    std::optional<std::uint32_t> ReadIntegerAttribute(std::uint32_t,std::string_view)override;
    void GetActiveCamera(std::uint32_t)override;
    std::uint32_t CameraManager()override;
    std::uint32_t UICameraName()override;
    void ActivateCamera(std::uint32_t,std::uint32_t)override;
    std::uint32_t SmithSystem()override;
    void SetSmithFlag(std::uint32_t,bool)override;
    void SetSmithState(std::uint32_t,std::uint32_t)override;
    void ClearSmithFlags(std::uint32_t)override;
    void UpdateUI(std::uint32_t,double)override;
    std::uint32_t RenderEffectTransform(std::uint32_t,double)override;
    void CleanupEffects(std::uint32_t)override;
private:
    void WriteByte(std::uint32_t,std::uint8_t)override;
    std::uint8_t DispatchEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
    void CheckSmith(std::uint32_t)const;
    ISystemMenuRuntime& ui_;GameRenderState& game_;std::uint8_t& flag_;ConfigReader config_;
};
}
