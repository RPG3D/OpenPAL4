#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActorRosterStateQueryRuntime {
public:
    virtual ~ICombatActorRosterStateQueryRuntime()=default;
    virtual std::uint32_t FirstPlayerIterator()=0;
    virtual std::uint32_t FirstMonsterIterator()=0;
    virtual bool IteratorAtEnd(std::uint32_t)=0;
    virtual std::uint32_t IteratorCurrent(std::uint32_t)=0;
    virtual void IteratorAdvance(std::uint32_t)=0;
    virtual void DestroyIterator(std::uint32_t)=0;
    virtual bool IsActorReady(std::uint32_t)=0;
    virtual bool IsStateZero(std::uint32_t)=0;
};
class ICombatActorStateQueryRuntime {
public:
    virtual ~ICombatActorStateQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t FirstEvent37Iterator(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t FirstMonsterIterator(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IsStateZero(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class ICombatActorReadyRosterRuntime : public ICombatActorRosterStateQueryRuntime {
public:
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
};

class CombatActorStateQueries final {
public:
    // IDA 0x57b9b0.
    [[nodiscard]] static std::uint32_t CheckExtended(
        ICombatActorStateQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CheckExtended(
        ICombatActorRosterStateQueryRuntime& runtime);
    // IDA 0x57bb20.
    [[nodiscard]] static bool CheckActorsReady(
        ICombatActorStateQueryRuntime& runtime);
    [[nodiscard]] static bool CheckActorsReady(ICombatActorReadyRosterRuntime&);
};

}  // namespace pal4
