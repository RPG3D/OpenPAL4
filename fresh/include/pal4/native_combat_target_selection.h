#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <functional>
namespace pal4 {
class NativeScriptEventRouter;
struct NativeCombatTargetSelectionImage final {std::array<std::uint32_t,21> words{};};
static_assert(sizeof(NativeCombatTargetSelectionImage)==84u);
// Required game services; mode and refresh must execute the actual roster/DB
// algorithms. No success defaults. Router/storage lifetime belongs to the owner.
class INativeCombatTargetSelectionServices {
public:
    virtual ~INativeCombatTargetSelectionServices()=default;
    virtual std::uint32_t CombatSystemAddress()=0;
    virtual std::uint32_t InputManagerAddress()=0;
    virtual std::uint32_t DetermineTargetMode(std::uint32_t controller)=0;
    virtual std::uint32_t RefreshSelection(std::uint32_t controller)=0;
    virtual std::uint32_t InitializeInput(std::uint32_t input)=0;
    virtual std::uint32_t SetTargetSelected(std::uint32_t actor,std::uint32_t selected)=0;
};
// Owns the WorldManager base constructor and five original subscriptions.
// Borrowed image, router and services must outlive this owner. Full per-frame
// target projection/mouse search remain separate. Keyboard command control
// and production mixed roster services compose through explicit native owners.
class NativeCombatTargetSelection final {
public:
    NativeCombatTargetSelection(NativeScriptEventRouter&,INativeCombatTargetSelectionServices&);
    NativeCombatTargetSelection(NativeScriptEventRouter&,INativeCombatTargetSelectionServices&,
                                NativeCombatTargetSelectionImage&);
    ~NativeCombatTargetSelection();
    NativeCombatTargetSelection(const NativeCombatTargetSelection&)=delete;
    NativeCombatTargetSelection& operator=(const NativeCombatTargetSelection&)=delete;
    void Subscribe();
    std::uint32_t CurrentActor();
    std::uint32_t ConfirmTarget();
    std::uint32_t CancelTarget();
    //53BBD0 only; caller has already dispatched re_start_ui_control.
    std::uint32_t StopInputSelection();
    // Original53BA40 used by EndCombat; subscriptions and target pair remain.
    void ResetCommandState();
    // Pin this controller across pointer queries and synchronous visual events.
    std::uint32_t DispatchPointer(const std::function<std::uint32_t()>&);
    void Close();
    std::uint32_t Address() const noexcept;
    std::uint32_t SubscriptionCount() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
