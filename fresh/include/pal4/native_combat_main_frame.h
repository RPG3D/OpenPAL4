#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeCombatMainWindow;
class NativeScriptEventRouter;
class NativeEventHandlerArray;
class NativeGameTimerQueue;
struct NativeCombatMainFrameServices {
    // Original callback entry IDs are dispatch keys, never callable addresses.
    std::function<void(std::uint32_t callback,std::uint32_t payload)> game_event;
    std::function<bool(std::uint32_t event)> key_down,root_click;
};
//54C1E0 lifecycle over actual native router/activity/timer owners. Borrowers
// must outlive this owner; Close before the Main window or UI session stops.
// Consumers are required: this class does not supply battle action semantics.
class NativeCombatMainFrame final {
public:
    NativeCombatMainFrame(NativeCombatMainWindow&,NativeCeguiUiSession&,
        NativeScriptEventRouter&,NativeEventHandlerArray&,NativeGameTimerQueue&,
        std::uint32_t frame_manager,NativeCombatMainFrameServices);
    ~NativeCombatMainFrame();
    NativeCombatMainFrame(const NativeCombatMainFrame&)=delete;
    NativeCombatMainFrame& operator=(const NativeCombatMainFrame&)=delete;
    void SetVisible(bool visible,std::uint32_t parent=0u);
    std::uint32_t InputAddress() const noexcept;
    std::uint32_t InvokeVirtual(std::uint32_t object,std::uint32_t slot);
    std::uint8_t InvokePredicate(std::uint32_t object,std::uint32_t slot,std::uint32_t event);
    std::size_t SubscriptionCount() const;
    void Close();
private:
    class Impl;
    std::shared_ptr<Impl> impl_;
};
}
