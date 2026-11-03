#pragma once
#include "pal4/native_combat_target_command_services.h"
#include "pal4/actor_manager_target_selection_controller.h"
#include <cstddef>
#include <vector>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatPlayerModelCache;
class NativeCombatActorSkillRuntime;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
// Mixed actor464/472 stage cursors and actual Player/Monster selection visuals.
// Register new Player caches/skills before their new-actor event. Close before
// any registered actor/cache or the effect session retires.
class NativeCombatRosterTargets final : public INativeCombatTargetSelectionActors,
    public IActorManagerTargetCandidatesRuntime {
public:
    NativeCombatRosterTargets(std::uint32_t combat,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,NativeEffectSceneSession&,NativeScriptEventRouter&);
    ~NativeCombatRosterTargets();
    void RegisterPlayer(NativeCombatPlayerModelCache&,NativeCombatActorSkillRuntime&);
    void Close();
    std::size_t ActiveIterators() const noexcept;
    // Checked52BC50 actor808 -> model20, and typed stage enumeration.
    std::uint32_t ActorClumpAddress(std::uint32_t actor);
    std::uint32_t CreateStageIterator(bool player);
    //52CC40: first roster, filtered by actor20 fundamental element relation.
    std::vector<std::uint32_t> CollectRelated(std::uint32_t actor);
    std::vector<std::uint32_t> CollectAll(std::uint32_t actor);
    // IDA52B410: actual actor472 opposite-stage scan and property800 slot4.
    bool ProcessReadyCandidate(std::uint32_t actor);
    std::uint32_t CombatSystemAddress() override;
    std::uint32_t RefreshSelection(std::uint32_t) override;
    std::uint32_t SetTargetSelected(std::uint32_t,std::uint32_t) override;
    std::uint32_t ActorNameAddress(std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    void WriteByte(std::uint32_t,std::uint8_t) override;
    std::uint32_t SelectionDialog(std::uint32_t) override;
    std::uint32_t CreateSelectionIterator(std::uint32_t,bool) override;
    std::uint32_t CloneSelectionIterator(std::uint32_t) override;
    void DeleteTemporaryIterator(std::uint32_t,std::uint32_t) override;
    bool IteratorHasData(std::uint32_t) override;
    bool IteratorAtEnd(std::uint32_t) override;
    std::uint32_t IteratorValue(std::uint32_t) override;
    void IteratorNext(std::uint32_t) override;
    void ResetSelectionIterator(std::uint32_t);
    std::uint32_t DeleteIterator(std::uint32_t,std::uint32_t) override;
    bool TargetStateNotThree(std::uint32_t) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
