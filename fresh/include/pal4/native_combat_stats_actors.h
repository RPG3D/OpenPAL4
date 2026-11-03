#pragma once
#include "pal4/combat_game_event_iterations.h"
#include <cstddef>
#include <memory>
#include <span>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatGameEventHandler;
// Actual game-stat actor queries and first-roster iteration. Both rosters and
// their skill owners outlive this binding. Unbind stats before destroying it.
class NativeCombatStatsActors final : public ICombatGameEventIterationRuntime {
public:
    NativeCombatStatsActors(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,NativeCombatGameEventHandler&);
    ~NativeCombatStatsActors();
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    std::uint32_t ActorFromFacet(std::uint32_t) override;
    std::uint32_t UpdateStat(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t GetActorManager() override;
    std::uint32_t CreateIterator(std::uint32_t) override;
    std::uint32_t IteratorDone(std::uint32_t) override;
    std::uint32_t IteratorCurrent(std::uint32_t) override;
    void IteratorAdvance(std::uint32_t) override;
    std::uint32_t DestroyIterator(std::uint32_t,std::uint32_t) override;
    std::uint32_t MonsterState(std::uint32_t) override;
    std::uint32_t ActorProperty52(std::uint32_t,std::uint32_t) override;
    std::uint32_t ActorProperty48(std::uint32_t,std::uint32_t) override;
    std::uint32_t ActorMetric252(std::uint32_t) override;
    std::int32_t AverageProperty4(std::uint32_t) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
