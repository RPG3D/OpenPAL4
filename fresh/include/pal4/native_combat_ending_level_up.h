#pragma once
#include "pal4/actor_render_state.h"
#include <array>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatEndingWindow;class NativeCombatPlayerManager;class NativeCombatActorSkillRuntime;
class NativeCombatHints;class NativeCombatTaskQueue;class NativeCombatResultActions;
class NativeMilesSound2dRuntime;class NativeMagicWindowRecords;class NativeGameDatabase;
struct NativeCombatSystemImage;
// Original542F00 over owned report IDs, real Stunt metadata, DLL hints and
// existing result-action/effect services. Borrowed owners outlive Close.
class NativeCombatEndingLevelUp final {
public:
    using Skills=std::function<NativeCombatActorSkillRuntime&(std::uint32_t)>;
    using Actions=std::function<NativeCombatResultActions&(std::uint32_t)>;
    using Spawn=std::function<ActorEffectHandle(std::uint32_t,std::string_view,std::string_view)>;
    NativeCombatEndingLevelUp(NativeCombatEndingWindow&,NativeCombatPlayerManager&,NativeCombatHints&,
        NativeCombatTaskQueue&,NativeMilesSound2dRuntime&,const NativeMagicWindowRecords&,const NativeGameDatabase&,
        NativeCombatSystemImage&,Skills,Actions,Spawn);
    ~NativeCombatEndingLevelUp();
    void Apply(std::uint32_t row,std::int32_t level,std::uint32_t actor,std::uint32_t hint,std::uint32_t record);
    // Borrowed original one-shot IDs for diagnostics; sound runtime owns/reaps them.
    std::array<std::uint32_t,2> LastSamples()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
