#pragma once
#include "pal4/trade_state.h"
#include <functional>
namespace pal4 {
class ISystemMenuRuntime;
class NativeCameraManagerOwner;
class NativeTradeSystemOwner;
struct NativeTradeStateServices {
    std::function<std::uint32_t(std::uint32_t)> frame_camera;
    std::function<void(std::uint32_t)> world_frame;
    std::function<void(double)> wait_handlers;
    std::function<void()> close_shop;
    std::function<void(std::uint32_t)> bind_ui_camera;
};
// Borrows one UI backend, the real trade owner and native camera controller.
// World rendering and script waits remain with their existing application owners.
class NativeTradeStateRuntime final : public ITradeStateRuntime {
public:
    NativeTradeStateRuntime(ISystemMenuRuntime&, NativeCameraManagerOwner&,
        NativeTradeSystemOwner&, std::uint32_t arena, std::uint32_t executor,
        NativeTradeStateServices);
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t FrameCamera(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    std::uint32_t TradeSystem() override;
    std::uint32_t ArenaManager() override;
    void ProcessWorldFrame(std::uint32_t,std::uint32_t) override;
    std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t,std::uint32_t) override;
    void PreRenderUI(std::uint32_t,std::uint32_t) override;
    std::uint32_t RenderUI(std::uint32_t,std::uint32_t) override;
    void UpdateUI(std::uint32_t,double) override;
    std::uint32_t Console() override;
    std::uint32_t ScriptExecutor() override;
    void ProcessWaitHandlers(std::uint32_t,double) override;
    std::uint32_t CameraManager() override;
    std::uint32_t MainCameraName() override;
    std::uint32_t UICameraName() override;
    std::uint32_t ActivateCamera(std::uint32_t,std::uint32_t) override;
    std::uint32_t RenderEffectTransform(std::uint32_t,double) override;
    void CloseShop(std::uint32_t) override;
    void CleanupEffects(std::uint32_t) override;
private:
    ISystemMenuRuntime& ui_;
    NativeCameraManagerOwner& cameras_;
    NativeTradeSystemOwner& trade_;
    std::uint32_t arena_,executor_;
    NativeTradeStateServices services_;
};
}
