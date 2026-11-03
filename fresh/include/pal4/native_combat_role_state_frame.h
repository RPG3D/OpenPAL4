#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
namespace pal4 {
class NativeCeguiUiSession;class NativeCombatPlayerManager;
class NativeCombatSkillActorRegistry;class NativeScriptEventRouter;
class NativeCombatTimedEvents;struct NativeCombatSystemImage;
struct PlayerManagerObjectState;
// Borrows the524-byte frame and its existing Elements owner. Close before
// roster/skills/timers/layout retirement. Click game consumers remain separate.
class NativeCombatRoleStateFrame final {
public:
    NativeCombatRoleStateFrame(NativeCeguiUiSession&,std::span<std::uint32_t,131>,
        NativeCombatPlayerManager&,NativeCombatSkillActorRegistry&,NativeScriptEventRouter&,
        NativeCombatSystemImage&,NativeCombatTimedEvents&,PlayerManagerObjectState&);
    ~NativeCombatRoleStateFrame();
    NativeCombatRoleStateFrame(const NativeCombatRoleStateFrame&)=delete;
    NativeCombatRoleStateFrame& operator=(const NativeCombatRoleStateFrame&)=delete;
    std::uint32_t Address()const noexcept;
    void SetVisible(bool,std::uint32_t parent=0u);
    void RefreshActor(std::uint32_t);
    void RefreshStatus(std::uint32_t);
    void RefreshColours();
    void DispatchTimer(std::uint32_t owner,std::uint32_t method);
    std::size_t SubscriptionCount()const noexcept;
    std::size_t ActiveCursors()const noexcept;
    std::size_t ColourTicks()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
