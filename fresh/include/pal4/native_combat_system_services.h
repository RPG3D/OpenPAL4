#pragma once
#include "pal4/native_combat_system_manager.h"
#include <functional>
#include <memory>
#include <span>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatFormulaCatalog;
class INativeSkillSystemServices;
class INativeCombatPlayerTurnServices;
// Borrows the actual two managers, their skills and CombatSystem storage.
// Retire the SystemManager before these services and all their dependencies.
// This composes scheduling only; it does not initialize Combat or stage actors.
class NativeCombatSystemServices final : public INativeCombatSystemManagerServices {
public:
    using TurnResolver=std::function<INativeCombatPlayerTurnServices&(std::uint32_t actor)>;
    NativeCombatSystemServices(NativeCombatSystemImage&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const>,
        INativeSkillSystemServices&,const NativeCombatFormulaCatalog&,
        TurnResolver,std::function<void()> touch_global);
    // Retained for single-service fixtures. Production supplies an actor resolver
    // so model animation and death queues belong to the selected actor.
    NativeCombatSystemServices(NativeCombatSystemImage&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const>,
        INativeSkillSystemServices&,const NativeCombatFormulaCatalog&,
        INativeCombatPlayerTurnServices&,std::function<void()> touch_global);
    ~NativeCombatSystemServices() override;
    std::size_t ActiveIterators() const noexcept;
    // Register the actual newly inserted offstage Player's skill owner before
    // re_new_added. The owner outlives these services. No active cursor/rebind.
    void BindPlayerSkills(NativeCombatActorSkillRuntime&);
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    std::uint32_t DialogDataFromParam(std::uint32_t) override;
    std::uint32_t InternalObject(std::uint32_t) override;
    std::uint32_t CombatSystemAddress() override;
    bool CombatInactive() override;
    float ObjectRate(std::uint32_t) override;
    float ObjectProperty(std::uint32_t) override;
    void SetObjectProperty(std::uint32_t,float) override;
    std::uint32_t CombatState() override;
    void SetCombatState(std::uint32_t) override;
    void TouchSkillSystem() override;
    void TouchGlobalSystem() override;
    bool ObjectReady(std::uint32_t) override;
    std::uint32_t BeginActors(std::uint32_t) override;
    bool IteratorFinished(std::uint32_t) override;
    std::uint32_t IteratorCurrent(std::uint32_t) override;
    void IteratorAdvance(std::uint32_t) override;
    std::uint8_t ActorReady(std::uint32_t) override;
    void ReleaseActorIterator(std::uint32_t) override;
    void PauseManager(std::uint32_t) override;
    void StartObject(std::uint32_t) override;
    void InvokeManagerSlot4(std::uint32_t) override;
    std::uint32_t ReleaseEvents(const std::vector<std::uint32_t>&) override;
    void PrepareObject(std::uint32_t) override;
    float GameDbFloat(std::uint32_t) override;
    std::uint32_t RandomNumber() override;
    std::uint32_t ObjectGroup(std::uint32_t) override;
    std::uint32_t ComparisonRandomIndex() const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
