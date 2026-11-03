#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatEndingWindow;class NativeCombatEndingTasks;class NativeCombatPlayerManager;
class NativeCombatTaskQueue;class NativeCombatActorSkillRuntime;class NativeCombatResultActions;
class NativeUpgradeRecordCatalog;struct NativeCombatSystemImage;
class INativeCombatActorTurnServices;
class NativeCombatConsole;
// Original547270 composer. Tasks, Players, frame and battle queue remain the
// sole owners of their storage; all borrowed dependencies outlive this service.
class NativeCombatEndingSequence final {
public:
    using Skills=std::function<NativeCombatActorSkillRuntime&(std::uint32_t)>;
    using Actions=std::function<NativeCombatResultActions&(std::uint32_t)>;
    using Visuals=std::function<INativeCombatActorTurnServices&(std::uint32_t)>;
    using LevelUp=std::function<void(std::uint32_t,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)>;
    NativeCombatEndingSequence(NativeCombatEndingWindow&,NativeCombatEndingTasks&,
        NativeCombatPlayerManager&,NativeCombatTaskQueue&,NativeCombatSystemImage&,
        const NativeUpgradeRecordCatalog&,Skills,Actions,Visuals={});
    ~NativeCombatEndingSequence();
    void Start();
    //547BD0/5481D0 input shortcuts, sharing this exact parent/child owner.
    void ShowResult(NativeCombatConsole&,LevelUp);
    void HideResult(NativeCombatConsole&);
    // Borrowed last published root; valid only while its queue retains it.
    std::uint32_t LastSequence()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
