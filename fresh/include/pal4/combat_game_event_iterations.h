#pragma once

#include "pal4/combat_game_event_callbacks.h"

#include <cstdint>

namespace pal4 {

class ICombatGameEventIterationRuntime :
    public ICombatGameEventCallbackRuntime {
public:
    [[nodiscard]] virtual std::uint32_t GetActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorDone(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyIterator(
        std::uint32_t iterator, std::uint32_t deleting_flag) = 0;
    [[nodiscard]] virtual std::uint32_t MonsterState(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorProperty52(
        std::uint32_t actor, std::uint32_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t ActorProperty48(
        std::uint32_t actor, std::uint32_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t ActorMetric252(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t AverageProperty4(
        std::uint32_t collection) = 0;
};

class CombatGameEventIterations final {
public:
    // IDA 0x5748c0.
    [[nodiscard]] static std::uint32_t OnMonsterFlee(
        std::uint32_t object, ICombatGameEventIterationRuntime& runtime);
    // IDA 0x574970.
    [[nodiscard]] static std::uint32_t OnBattleEnd(
        std::uint32_t object, ICombatGameEventIterationRuntime& runtime);
};

}  // namespace pal4
