#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerMonsterGateRuntime {
public:
    virtual ~IActorManagerPlayerMonsterGateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t FirstIterator(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual bool IteratorDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorValue(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool MonsterSlot412(
        std::uint32_t monster) = 0;
    [[nodiscard]] virtual bool CombatActive() = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot532(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot536(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t DestroyIterator(
        std::uint32_t iterator, std::uint32_t flags) = 0;
};

class ActorManagerPlayerMonsterGate final {
public:
    [[nodiscard]] static std::int32_t SelectAction(
        std::uint32_t actor, IActorManagerPlayerMonsterGateRuntime& runtime);
};

}  // namespace pal4
