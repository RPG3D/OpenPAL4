#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatTargetSelection;class NativeCombatActorManagerOwner;
class NativeEffectSceneSession;class NativeCombatConsole;class NativeCombatSystemTail;
class NativeCombatTaskQueue;
//575ED0 preserves session/timer/UI owners. Roster services retire actor borrowers
//before deleting actors; restore_render must bind the captured host callback.
class NativeCombatEndCombat final {
public:
    NativeCombatEndCombat(NativeCombatSystemImage&,NativeCombatTargetSelection&,
        NativeCombatActorManagerOwner&,NativeEffectSceneSession&,NativeCombatConsole&,NativeCombatSystemTail&,NativeCombatTaskQueue&,
        std::function<void()> clear_rosters,std::function<void(std::uint32_t)> restore_render);
    ~NativeCombatEndCombat();
    std::uint32_t CombatSystemAddress() const noexcept;
    void Run();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
