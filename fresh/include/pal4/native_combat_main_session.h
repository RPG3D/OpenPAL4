#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatMainUiEvents;
class NativeCombatMainActionReady;
class NativeCombatMainTooltips;
class NativeCombatMainJointHint;
class NativeCombatMainJointSelection;
class NativeCombatMainInput;
class NativeCeguiUiSession;
class NativeScriptEventRouter;
class NativeEventHandlerArray;
class NativeGameTimerQueue;
struct NativeCombatMainFrameServices;
// Composes the original19-event frame with actual game consumers. It owns
// transport/lifecycle only; all borrowed consumers and UI owners outlive it.
class NativeCombatMainSession final {
public:
    NativeCombatMainSession(NativeCombatMainWindow&,NativeCombatMainUiEvents&,NativeCombatMainActionReady&,
        NativeCombatMainTooltips&,NativeCombatMainJointHint&,NativeCombatMainJointSelection&,NativeCombatMainInput&,
        NativeCeguiUiSession&,NativeScriptEventRouter&,NativeEventHandlerArray&,NativeGameTimerQueue&,std::uint32_t frame_manager);
    // Lifecycle can precede actor creation. Consumers resolve the current
    // production owners on dispatch; they must report missing services.
    NativeCombatMainSession(NativeCombatMainWindow&,NativeCeguiUiSession&,
        NativeScriptEventRouter&,NativeEventHandlerArray&,NativeGameTimerQueue&,
        std::uint32_t frame_manager,NativeCombatMainFrameServices);
    ~NativeCombatMainSession();
    NativeCombatMainSession(const NativeCombatMainSession&)=delete;
    NativeCombatMainSession& operator=(const NativeCombatMainSession&)=delete;
    static bool Supports(std::uint32_t callback) noexcept;
    void SetVisible(bool,std::uint32_t parent=0u);
    std::uint32_t InputAddress() const noexcept;
    bool OwnsActivity(std::uint32_t) const noexcept;
    std::uint32_t InvokeVirtual(std::uint32_t object,std::uint32_t slot);
    std::uint8_t InvokePredicate(std::uint32_t object,std::uint32_t slot,std::uint32_t event);
    std::size_t SubscriptionCount() const;
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
