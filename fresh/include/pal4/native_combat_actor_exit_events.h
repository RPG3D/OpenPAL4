#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;class NativeCombatEventHandlers;class NativeCombatSkillActorRegistry;
class NativeCombatPlayerManager;class NativeCombatMonsterManager;class NativeCombatRosterTargets;
class NativeEffectSceneSession;class NativePlayerCollectionCatalog;class NativeScriptEventRouter;
class NativeCombatPlayerModelCache;class NativeCombatPlayerMotion;
struct NativeCombatPlayerExitServices final {
    std::function<NativeCombatPlayerModelCache&(std::uint32_t)> cache;
    NativeCombatPlayerMotion* motion{};
};
//Original57B220/57B710 share roster queries, replacement and virtual388.
//Death uses actual544 context; Fled uses actual532 context. Pins first-roster owners,
//permits source Monster payload/stage retirement, then runs original outcome
//gates. Borrowed services outlive Handle. Progress callback must store the real
//global-data key; an unavailable backend must fail explicitly.
class NativeCombatActorExitEvents final {
public:
    NativeCombatActorExitEvents(NativeCombatSystemImage&,NativeCombatEventHandlers&,NativeCombatSkillActorRegistry&,
        NativeCombatPlayerManager&,NativeCombatMonsterManager&,NativeCombatRosterTargets&,NativeEffectSceneSession&,
        const NativePlayerCollectionCatalog&,NativeScriptEventRouter&,std::function<void(std::uint32_t)> store_progress,
        NativeCombatPlayerExitServices player_services={});
    ~NativeCombatActorExitEvents();
    NativeCombatActorExitEvents(const NativeCombatActorExitEvents&)=delete;
    NativeCombatActorExitEvents& operator=(const NativeCombatActorExitEvents&)=delete;
    void Handle(std::uint32_t owner,std::uint32_t payload);
    void HandleFled(std::uint32_t owner,std::uint32_t payload);
    void Close();
private:
    void HandleExit(std::uint32_t owner,std::uint32_t payload,bool fled);
    class Impl;std::unique_ptr<Impl> impl_;
};
}
