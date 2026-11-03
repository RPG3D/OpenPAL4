#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <functional>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatAttackActions;
class NativeCombatDefendActions;
class NativeCombatFleeActions;
class NativeMagicWindowRecords;class NativeCombatSkillDomain;
class NativeInventorySession;class NativeCombatPropertyDomain;
class NativeCombatTaskQueue;
class NativeCombatPlayerActionMemory;
class NativeCombatEventHandlers;
class NativeScriptEventRouter;
struct NativeCombatSystemImage;
//57C4B0 action dispatch over real Player/queue/template owners. The existing
// Attack, optional Defend/Flee and bound Magic domains supply real actions.
// Borrowed Magic records/domains outlive commands, queue and replay owners.
// Skill and Property families require explicit borrowed execution services.
class NativeCombatCommandActions final {
public:
    NativeCombatCommandActions(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,NativeCombatAttackActions&,
        NativeCombatTaskQueue&,NativeCombatPlayerActionMemory&,NativeCombatEventHandlers&,
        NativeCombatSystemImage&,NativeScriptEventRouter&,NativeCombatDefendActions* = nullptr,NativeCombatFleeActions* = nullptr);
    ~NativeCombatCommandActions();
    using MagicResolver=std::function<NativeCombatSkillDomain&(std::uint32_t)>;
    void BindMagic(const NativeMagicWindowRecords&,MagicResolver);
    using StuntResolver=std::function<NativeCombatSkillDomain&(std::uint32_t)>;
    void BindStunt(const NativeMagicWindowRecords&,StuntResolver);
    using PropertyResolver=std::function<NativeCombatPropertyDomain&(std::uint32_t)>;
    void BindProperty(NativeInventorySession&,PropertyResolver);
    std::uint32_t Handle(std::uint32_t owner,std::uint32_t payload);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
