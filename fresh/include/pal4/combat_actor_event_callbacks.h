#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActorFirstRosterRuntime {
public:
    virtual ~ICombatActorFirstRosterRuntime()=default;
    virtual std::uint32_t CreateCandidateIterator(std::uint32_t)=0;
    virtual bool IteratorAtEnd(std::uint32_t)=0;
    virtual std::uint32_t IteratorCurrent(std::uint32_t)=0;
    virtual void IteratorAdvance(std::uint32_t)=0;
    virtual void DestroyIterator(std::uint32_t)=0;
    virtual bool IsActorReady(std::uint32_t)=0;
    virtual bool IsStateZero(std::uint32_t)=0;
};
class ICombatActorReplacementRuntime : public ICombatActorFirstRosterRuntime {
public:
    virtual std::int32_t ActorSlot(std::uint32_t)=0;
    virtual void RetireReplacementActor(std::uint32_t)=0;
    virtual void InitializeEffects(std::uint32_t,std::int32_t)=0;
};

// Narrow57B710 service boundary, independent from rotation and action-end APIs.
class ICombatActorFledRuntime {
public:
    virtual ~ICombatActorFledRuntime()=default;
    virtual std::uint32_t ActorFromFacet(std::uint32_t)=0;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual bool IsLastActor(std::uint32_t)=0;
    virtual void SetActorState(std::uint32_t,std::uint32_t,std::uint32_t)=0;
    virtual std::uint8_t FinalizeActor(std::uint32_t)=0;
    virtual bool ReplaceActor(std::uint32_t)=0;
    virtual std::uint32_t DispatchResult(std::uint32_t)=0;
};
class ICombatActorActionEndRuntime {
public:
    virtual ~ICombatActorActionEndRuntime()=default;
    virtual std::uint32_t ActorFromFacet(std::uint32_t event)=0;
    virtual std::uint32_t CombatSystem()=0;
    virtual std::uint32_t CreateExtendedIterator(std::uint32_t actor)=0;
    virtual bool IteratorAtEnd(std::uint32_t iterator)=0;
    virtual std::uint32_t IteratorCurrent(std::uint32_t iterator)=0;
    virtual void IteratorAdvance(std::uint32_t iterator)=0;
    virtual void DestroyIterator(std::uint32_t iterator)=0;
    virtual bool ObjectStateNotThree(std::uint32_t actor)=0;
    virtual std::uint32_t ActorVisualMode(std::uint32_t actor)=0;
    virtual void SetVisualScalar(std::uint32_t facet,std::uint32_t bits)=0;
    virtual std::uint32_t CheckActorStateExtended(std::uint32_t owner)=0;
    virtual std::uint8_t ReadByte(std::uint32_t address)=0;
    virtual void WriteByte(std::uint32_t address,std::uint8_t value)=0;
    virtual bool IsCombatStateActive(std::uint32_t system)=0;
    virtual std::uint32_t ScriptEventRouter()=0;
    virtual std::uint32_t ProcessScriptEvent(std::uint32_t router,std::uint32_t descriptor,
        std::uint32_t payload,std::uint32_t mode)=0;
};
class ICombatActorEventRuntime : public ICombatActorActionEndRuntime,public ICombatActorReplacementRuntime {
public:
    virtual bool IteratorAtEnd(std::uint32_t)=0;
    virtual std::uint32_t IteratorCurrent(std::uint32_t)=0;
    virtual void IteratorAdvance(std::uint32_t)=0;
    virtual void DestroyIterator(std::uint32_t)=0;
    void RetireReplacementActor(std::uint32_t a)override{(void)FinalizeActor(a);}
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value)=0;
    virtual std::int32_t ActorSlot(std::uint32_t actor)=0;
    virtual std::uint8_t FinalizeActor(std::uint32_t actor)=0;
    virtual std::uint32_t CreateCandidateIterator(std::uint32_t actor)=0;
    virtual bool IsStateZero(std::uint32_t actor)=0;
    virtual bool IsActorReady(std::uint32_t actor)=0;
    virtual void InitializeEffects(std::uint32_t actor,std::int32_t slot)=0;
    virtual std::uint32_t ActorRotation(std::uint32_t actor,std::uint32_t mode)=0;
    virtual void ProcessChildSlot4(std::uint32_t actor,std::uint32_t rotation,std::uint32_t mode)=0;
    virtual std::uint32_t ProcessRotationResult(std::uint32_t actor,std::uint32_t mode)=0;
    virtual bool CheckStateThree(std::uint32_t actor)=0;
    virtual std::uint32_t ActorId(std::uint32_t actor)=0;
    virtual void SetActorState(std::uint32_t actor,std::uint32_t state,std::uint32_t mode)=0;
};

class CombatActorEventCallbacks final {
public:
    // IDA 0x57b420.
    [[nodiscard]] static bool HandleActorEvent(
        std::uint32_t actor, ICombatActorReplacementRuntime& runtime);
    // IDA 0x57b520.
    [[nodiscard]] static std::uint32_t GetRotationAndProcess(
        std::uint32_t event, ICombatActorEventRuntime& runtime);
    // IDA 0x57b570.
    static void HandleStateChange(
        std::uint32_t event, ICombatActorEventRuntime& runtime);
    // IDA 0x57b5e0.
    [[nodiscard]] static std::uint32_t HandleExtended(
        std::uint32_t owner, std::uint32_t event,
        ICombatActorActionEndRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleFled(std::uint32_t event,ICombatActorFledRuntime& runtime);
    // IDA 0x57b710.
    [[nodiscard]] static std::uint8_t HandleType(
        std::uint32_t owner, std::uint32_t event,
        ICombatActorEventRuntime& runtime);
    // IDA 0x57b8e0.
    [[nodiscard]] static bool CheckActorState(
        std::uint32_t actor, ICombatActorFirstRosterRuntime& runtime);
};

}  // namespace pal4
