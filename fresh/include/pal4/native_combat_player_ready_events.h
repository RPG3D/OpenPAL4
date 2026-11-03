#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatAiActions;class NativeCombatPlayerMotion;
class NativeCombatSkillActorRegistry;class NativeCombatRosterTargets;
class NativeCombatEventHandlers;class NativeCombatSystemAccess;struct NativeCombatSystemImage;
class NativeScriptEventRouter;
class NativeCombatPlayerReadyEvents final {
public:
    NativeCombatPlayerReadyEvents(NativeCombatAiActions&,NativeCombatPlayerMotion&,
        NativeCombatSkillActorRegistry&,NativeCombatRosterTargets&,NativeCombatEventHandlers&,
        NativeCombatSystemAccess&,NativeCombatSystemImage&,NativeScriptEventRouter&);
    ~NativeCombatPlayerReadyEvents();
    void Handle(std::uint32_t owner,std::uint32_t payload);
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
