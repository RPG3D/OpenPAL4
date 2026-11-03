#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActionFieldsRuntime {
public:
    virtual ~ICombatActionFieldsRuntime()=default;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value)=0;
};
class ICombatActionLifecycleRuntime : public ICombatActionFieldsRuntime {
public:
    virtual ~ICombatActionLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromFacet(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(std::uint32_t collection) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint8_t DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool CheckStateThree(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IsStateZero(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    virtual void SetActorState(std::uint32_t actor, std::uint32_t state,
                               std::uint32_t mode) = 0;
    virtual void FinalizeActor(std::uint32_t actor) = 0;
    virtual void InitializeEffects(std::uint32_t actor, std::int32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t GetOwnedChild(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFollowupTask(
        std::uint32_t child, std::uint32_t actor) = 0;
    virtual void RegisterTask(std::uint32_t queue, std::uint32_t task) = 0;
    virtual void ResetActionComponent(std::uint32_t component) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    virtual void DispatchEvent(std::uint32_t router, std::uint32_t descriptor,
                               std::uint32_t event, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool IsCombatStateEnding(std::uint32_t system) = 0;
};

class CombatActionLifecycle final {
public:
    // IDA 0x57c6b0.
    [[nodiscard]] static std::uint32_t ResetEventData(
        std::uint32_t owner, ICombatActionFieldsRuntime& runtime) {
        runtime.WriteWord(owner+64u,0u);runtime.WriteWord(owner+68u,0u);return 0u;
    }
    // IDA 0x57c6c0.
    [[nodiscard]] static std::uint8_t ProcessExtended(
        std::uint32_t owner, std::uint32_t event,
        ICombatActionLifecycleRuntime& runtime);
    // IDA 0x57c840.
    static void ProcessFinal(std::uint32_t owner, std::uint32_t event,
                             ICombatActionLifecycleRuntime& runtime);
    // IDA 0x57c920.
    [[nodiscard]] static std::uint32_t SetStateToFive(
        ICombatActionLifecycleRuntime& runtime);
};

}  // namespace pal4
