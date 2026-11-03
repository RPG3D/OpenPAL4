#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeScriptEventRouter;
// Owns the140/76-byte handler facades at CombatSystem404/408. Uses the shared
// original binding catalogs and native script bus; no legacy list/tree image.
// Required domain dispatch receives owner/entry/payload. Only verified57B8D0
// RET callbacks are handled without it. Slots/router outlive this owner.
class NativeCombatEventHandlers final {
public:
    using Dispatch=std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>;
    NativeCombatEventHandlers(NativeScriptEventRouter&,std::uint32_t& all_slot,std::uint32_t& extended_slot,Dispatch);
    ~NativeCombatEventHandlers();
    NativeCombatEventHandlers(const NativeCombatEventHandlers&)=delete;
    NativeCombatEventHandlers& operator=(const NativeCombatEventHandlers&)=delete;
    std::uint32_t AllAddress() const;
    std::uint32_t ExtendedAddress() const;
    std::size_t SubscriptionCount() const;
    // Original all-owner byte136 used by57B5E0, separate from its17 routes.
    bool EndCheckSeen() const;
    void SetEndCheckSeen(bool);
    std::uint32_t ReadyActor()const;
    void ResetControlData();
    void SetReadyActor(std::uint32_t);
    std::uint32_t ControlActor()const;
    void SetControlActor(std::uint32_t);
    bool UiControlStarted()const;
    void SetUiControlStarted(bool);
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
