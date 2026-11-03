#pragma once

#include <cstdint>

namespace pal4 {

//57B7B0 required game services, without an invented cbActorManager facade.
//Native event consumers ignore the original incidental iterator-destructor AL.
class ICombatStateResetServices {
public:
    virtual ~ICombatStateResetServices() = default;
    virtual bool Active() = 0;
    virtual void TouchSkillSystem() = 0;
    virtual void ResetTaskFlags() = 0;
    virtual void CleanupTaskQueue() = 0;
    virtual std::uint32_t FirstMonsterIterator() = 0;
    virtual bool IteratorAtEnd(std::uint32_t) = 0;
    virtual std::uint32_t IteratorCurrent(std::uint32_t) = 0;
    virtual void IteratorAdvance(std::uint32_t) = 0;
    virtual void DestroyIterator(std::uint32_t) = 0;
    virtual std::uint32_t ActorProperty(std::uint32_t,std::uint32_t) = 0;
    virtual void ApplyPropertyDelta(std::uint32_t,std::uint32_t,std::uint32_t) = 0;
    virtual void ResetActor(std::uint32_t,std::uint32_t) = 0;
    virtual void DispatchCombatOverOne() = 0;
};

class ICombatStateResetRuntime {
public:
    virtual ~ICombatStateResetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint8_t IsCombatStateActive(
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t SkillSystemInstance(
        std::uint32_t mode, const char* source, const char* action) = 0;
    virtual void NullSub91(std::uint32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    virtual void ResetTaskFlags(std::uint32_t queue) = 0;
    virtual void CleanupTaskQueue(std::uint32_t queue) = 0;
    [[nodiscard]] virtual std::uint32_t FirstMonsterIterator(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t ActorProperty(
        std::uint32_t actor, std::uint32_t index) = 0;
    virtual void ApplyPropertyDelta(std::uint32_t actor,
                                    std::uint32_t index,
                                    std::uint32_t delta_bits) = 0;
    virtual void ResetActor(std::uint32_t actor,
                            std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessScriptEvent(
        std::uint32_t router, std::uint32_t descriptor,
        std::uint32_t payload, std::uint32_t mode) = 0;
};

class CombatStateReset final {
public:
    static void Advance(ICombatStateResetServices&);
    // IDA 0x57b7b0.
    [[nodiscard]] static std::uint8_t Reset(
        std::uint32_t argument, ICombatStateResetRuntime& runtime);
    // IDA 0x57b8d0.
    static void NullSub3(std::uint32_t argument);
};

}  // namespace pal4
