#pragma once
#include <cstdint>
#include <memory>
#include <functional>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatTaskQueue;class NativeCombatMonsterManager;class NativeCombatEventHandlers;
class NativePlayerCollectionCatalog;class INativeSkillSystemServices;class NativeScriptEventRouter;
class INativeCombatActorTurnServices;
//57B7B0 handler. Borrows actual queue/first Monster roster/skills/property and
//event services. Victory report/rewards remain the combat_over consumer's job.
//Owners survive all synchronous actor/outcome callbacks; retire after Handle.
class NativeCombatQuickEnd final {
public:
    NativeCombatQuickEnd(NativeCombatSystemImage&,NativeCombatTaskQueue&,NativeCombatMonsterManager&,
        NativeCombatEventHandlers&,const NativePlayerCollectionCatalog&,INativeSkillSystemServices&,NativeScriptEventRouter&,
        std::function<INativeCombatActorTurnServices&(std::uint32_t)>);
    ~NativeCombatQuickEnd();
    NativeCombatQuickEnd(const NativeCombatQuickEnd&)=delete;
    NativeCombatQuickEnd& operator=(const NativeCombatQuickEnd&)=delete;
    void Handle(std::uint32_t handler_owner,std::uint32_t ignored_payload);
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
