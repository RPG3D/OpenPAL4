#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
namespace pal4 {
class NativeCombatSkillDomain;
class NativeMagicWindowRecords;
class NativeCombatAiDecisionRuntime;
class NativeCombatSkillActorRegistry;
class NativeCombatRosterTargets;
class NativeCombatAttackActions;
class NativeCombatFleeActions;
class NativeCombatTaskQueue;
class INativeSkillSystemServices;
class NativeScriptEventRouter;
// Original AI probability dispatch over the actual owned AI facet. Normal
// attacks use shared target/weight algorithms and the actual attack task queue.
// Monster pipelines reuse fixed skill lookup and original empty-candidate fallback.
// Actual Magic, Stunt and Flee use their prebound task domains. Original virtual
// slots16/20 select Summon/Flee respectively; they are not interchangeable.
// Other unbound branches remain explicit failures.
class NativeCombatAiActions final {
public:
    using MagicResolver = std::function<NativeCombatSkillDomain&(std::uint32_t)>;
    NativeCombatAiActions(NativeCombatAiDecisionRuntime&,NativeCombatSkillActorRegistry&,
        NativeCombatRosterTargets&,NativeCombatAttackActions&,NativeCombatFleeActions&,NativeCombatTaskQueue&,
        INativeSkillSystemServices&,NativeScriptEventRouter&,const NativeMagicWindowRecords&,MagicResolver,MagicResolver stunt);
    ~NativeCombatAiActions();
    bool Process(std::uint32_t actor);
    std::uint32_t LastSelectedTarget()const noexcept;
    std::uint32_t LastBranchSlot()const noexcept;
    std::size_t PendingPoolCount()const noexcept;
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
