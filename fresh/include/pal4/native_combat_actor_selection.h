#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatPlayerActorOwner;
class NativeCombatPlayerModelCache;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
// Actual52B570 Player/Monster pick/OM03 and ae_ui_select/unselect. Borrows actor/model,
// bound Combat effects and router. Close before actor/model/effect retirement;
// do not destroy the adapter during its callbacks. Both use actual owner guards.
class NativeCombatActorSelection final {
public:
    NativeCombatActorSelection(NativeCombatActorModelOwner&,NativeEffectSceneSession&,NativeScriptEventRouter&);
    NativeCombatActorSelection(NativeCombatPlayerActorOwner&,NativeCombatPlayerModelCache&,
        NativeEffectSceneSession&,NativeScriptEventRouter&);
    ~NativeCombatActorSelection();
    NativeCombatActorSelection(const NativeCombatActorSelection&)=delete;
    NativeCombatActorSelection& operator=(const NativeCombatActorSelection&)=delete;
    std::uint32_t SetSelected(std::uint32_t selected);
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
