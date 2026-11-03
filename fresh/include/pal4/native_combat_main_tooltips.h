#pragma once
#include <cstdint>
#include <span>
#include <vector>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCameraManagerOwner;
class NativeCeguiUiSession;
class WindowMessageConfig;
struct NativeCombatSystemImage;
//54CFD0 borrows actual state/mouse/actors/skills and delegates presentation to
// Main's original DLL controls. No subscriptions or actor ownership.
class NativeCombatMainTooltips final {
public:
    NativeCombatMainTooltips(NativeCombatMainWindow&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const>,
        NativeCombatSystemImage&,WindowMessageConfig&,NativeCameraManagerOwner&,NativeCeguiUiSession&);
    bool Handle(std::uint32_t payload);
private:
    NativeCombatMainWindow& window_;
    NativeCombatPlayerManager& players_;NativeCombatMonsterManager& monsters_;
    std::vector<NativeCombatActorSkillRuntime*> skills_;
    NativeCombatSystemImage& system_;WindowMessageConfig& config_;
    NativeCameraManagerOwner& cameras_;NativeCeguiUiSession& ui_;
};
}
