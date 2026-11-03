#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;class NativeTradeWindowPreview;
class NativeUiCollectionObjects;class NativeEffectSceneSession;
class NativePkFileManagerRuntime;class NativeEventHandlerArray;
struct NativeTradeLifecycleServices {
    // Cancel prompts/timers and detach owned list cells before the DLL reset.
    std::function<void()> before_hide;
    std::function<void()> reset_trade;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
};
class NativeTradeWindowLifecycle final {
public:
    NativeTradeWindowLifecycle(NativeTradeWindowLayout&,NativeTradeWindowPreview&,
        NativeUiCollectionObjects&,NativeEffectSceneSession&,NativePkFileManagerRuntime&,
        NativeEventHandlerArray&,std::uint32_t manager,NativeTradeLifecycleServices);
    ~NativeTradeWindowLifecycle();
    NativeTradeWindowLifecycle(const NativeTradeWindowLifecycle&)=delete;
    NativeTradeWindowLifecycle& operator=(const NativeTradeWindowLifecycle&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool RequestClose();
    bool Help();
    bool Visible() const noexcept;
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
