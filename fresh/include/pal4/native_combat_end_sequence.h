#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatPlayerManager;class NativeCombatPlayerWriteback;
class NativeCombatMainWindow;class NativeCombatMainSession;
class NativeCombatActionWindow;class NativeCombatActionFrame;class NativeCombatHints;
class NativeCombatEndingWindow;class NativeCombatConsole;
class NativeCombatRoleStateFrame;
//576440 orchestration; real UI/Player owners and final game lifecycle services
//are borrowed. A failing final lifecycle service remains an explicit boundary.
// Final callbacks must keep this owner, the executing timer and all borrowed
// services alive until Run returns; retire host owners after dispatch unwinds.
class NativeCombatEndSequence final {
public:
    NativeCombatEndSequence(NativeCombatSystemImage&,NativeCombatPlayerManager&,NativeCombatPlayerWriteback&,
        NativeCombatMainWindow&,NativeCombatMainSession&,NativeCombatActionWindow&,NativeCombatActionFrame&,
        NativeCombatHints&,NativeCombatEndingWindow&,NativeCombatConsole&,
        std::uint32_t frame_manager,std::function<void()> end_combat,
        std::function<std::uint32_t(std::string_view,std::uint32_t)> script_event);
    ~NativeCombatEndSequence();
    std::uint32_t CombatSystemAddress() const noexcept;
    //576600/576610 only: hide frames and clean both hints. Does not sync
    //players, change scheduler/result, end the battle or emit close_system.
    //Shares Run's reentry guard and borrowed-owner lifetime contract.
    void BindRoleFrame(NativeCombatRoleStateFrame*);
    std::uint32_t ShowUi();
    std::uint32_t CloseUi();
    std::uint32_t ShowEnding(std::uint32_t prize);
    std::uint32_t Run(std::uint32_t result);
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
