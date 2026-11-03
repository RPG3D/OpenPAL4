#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatActorManagerOwner;class NativeCombatPlayerManager;
class NativeCombatPlayerSceneActors;class NativeCombatMonsterManager;
class NativeCombatTaskQueue;class NativeCombatTimedEvents;class NativeCombatSystemManager;
class NativeCombatConsole;class NativeCombatWorldInput;class NativeEffectSceneSession;
class NativeCombatEndSequence;class NativeCombatEndCombat;class NativeCombatSkillActorRegistry;
class INativeSkillSystemServices;class NativeScriptEventRouter;
//Complete575F40 phases. All borrowed owners must survive synchronous timers,
//events and EndCombat, even when those clear actor payloads. Retire this adapter
//before those owners. This is not58AFD0 outcome/Scene transition or full Start.
class NativeCombatSystemUpdate final {
public:
    NativeCombatSystemUpdate(NativeCombatSystemImage&,NativeCombatActorManagerOwner&,
        NativeCombatPlayerManager&,NativeCombatPlayerSceneActors&,NativeCombatMonsterManager&,
        NativeCombatTaskQueue&,NativeCombatTimedEvents&,NativeCombatSystemManager&,
        NativeCombatConsole&,NativeCombatWorldInput&,NativeEffectSceneSession&,
        NativeCombatEndSequence&,NativeCombatEndCombat&,NativeCombatSkillActorRegistry&,
        INativeSkillSystemServices&,NativeScriptEventRouter&);
    ~NativeCombatSystemUpdate();
    NativeCombatSystemUpdate(const NativeCombatSystemUpdate&)=delete;
    NativeCombatSystemUpdate& operator=(const NativeCombatSystemUpdate&)=delete;
    void Advance(float seconds,std::uint32_t quick_end_payload_seed);
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
