#pragma once
#include "pal4/native_combat_target_command_services.h"
namespace pal4 {
class NativeCombatMonsterManager;
class NativeEffectSceneSession;
// Real Monster actor/roster half of the target-command boundary. Borrows the
// actual Combat image, manager and effects. Monster464 selects stage Monsters;
// Monster472 requires the separate Player candidate service and rejects here.
class NativeCombatMonsterTargetSelection final : public INativeCombatTargetSelectionActors {
public:
    NativeCombatMonsterTargetSelection(std::uint32_t combat,NativeCombatMonsterManager&,NativeEffectSceneSession&);
    ~NativeCombatMonsterTargetSelection() override;
    std::uint32_t CombatSystemAddress() override;
    std::uint32_t RefreshSelection(std::uint32_t controller) override;
    std::uint32_t SetTargetSelected(std::uint32_t actor,std::uint32_t value) override;
    std::uint32_t ActorNameAddress(std::uint32_t actor) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
