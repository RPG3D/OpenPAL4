#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
namespace pal4 {
class NativeCombatActionWindow;class NativeCombatPlayerManager;class NativeCombatMonsterManager;
class NativeCombatSkillActorRegistry;class NativeCombatRosterTargets;class NativeCombatTargetSelection;
class NativeScriptEventRouter;class NativeEventHandlerArray;
class NativeMagicWindowRecords;class NativeGameDatabase;class NativeInventorySession;
// Original Action keyboard, six-button click and navigation consumers.
// Magic and Property borrow actual catalogs/inventory through shared list
// services. Stunt borrows the actual learned collection and Player conditions.
class NativeCombatActionController final {
public:
    NativeCombatActionController(NativeCombatActionWindow&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,NativeCombatSkillActorRegistry&,NativeCombatRosterTargets&,
        NativeCombatTargetSelection&,NativeScriptEventRouter&,NativeEventHandlerArray&,std::uint32_t manager);
    ~NativeCombatActionController();
    bool Handle(std::uint32_t callback,std::uint32_t event);
    void BindMagicSelector(const NativeMagicWindowRecords&,const NativeGameDatabase&);
    void BindPropertySelector(NativeInventorySession&,const NativeGameDatabase&);
    void BindStuntSelector(const NativeMagicWindowRecords&,const NativeGameDatabase&);
    bool OwnsSelectorActivity(std::uint32_t)const noexcept;
    std::uint32_t InvokeSelectorVirtual(std::uint32_t,std::uint32_t);
    std::uint8_t InvokeSelectorPredicate(std::uint32_t,std::uint32_t,std::uint32_t);
    std::size_t ActiveIterators()const noexcept;
    std::size_t PendingPayloads()const noexcept;
    void Close();
private:
    class Impl;std::shared_ptr<Impl> impl_;
};
}
