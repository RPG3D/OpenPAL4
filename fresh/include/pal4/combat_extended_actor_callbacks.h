#pragma once

#include <cstdint>

namespace pal4 {

// Services consumed by the Monster57C370 callback. The extended Player
// callback adds its motion/UI operations below.
class ICombatMonsterReadyRuntime {
public:
    virtual ~ICombatMonsterReadyRuntime()=default;
    virtual std::uint32_t ActorFromFacet(std::uint32_t event)=0;
    virtual bool ProcessReadyCandidate(std::uint32_t actor)=0;
    virtual std::uint32_t CombatSystem()=0;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual void FinalizeActor(std::uint32_t actor)=0;
    virtual std::uint32_t ResetActionComponent(std::uint32_t component)=0;
    virtual std::uint32_t ActorId(std::uint32_t actor)=0;
    virtual std::uint32_t ActorSecondaryId(std::uint32_t actor)=0;
    virtual std::uint32_t ScriptEventRouter()=0;
    virtual std::uint32_t ProcessValueEvent(std::uint32_t router,std::uint32_t descriptor,
        std::uint32_t value,std::uint32_t mode)=0;
};
class ICombatPlayerReadyRuntime {
public:
    virtual ~ICombatPlayerReadyRuntime()=default;
    virtual std::uint32_t ActorFromFacet(std::uint32_t)=0;
    virtual bool ProcessReadyCandidate(std::uint32_t)=0;
    virtual std::uint32_t CombatSystem()=0;
    virtual bool IsCombatSystemInactive(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual void WriteByte(std::uint32_t,std::uint8_t)=0;
    virtual std::uint32_t ActorState(std::uint32_t)=0;
    virtual void ProcessActor(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t ScriptEventRouter()=0;
    virtual std::uint32_t ProcessActorEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)=0;
    virtual void FinalizeActor(std::uint32_t)=0;
    virtual std::uint32_t ResetActionComponent(std::uint32_t)=0;
};
class ICombatExtendedActorRuntime : public ICombatMonsterReadyRuntime,public ICombatPlayerReadyRuntime {
public:
    ~ICombatExtendedActorRuntime() override=default;
};

class CombatExtendedActorCallbacks final {
public:
    // IDA 0x57c290.
    [[nodiscard]] static std::uint32_t ProcessActorEvent(
        std::uint32_t owner, std::uint32_t event,
        ICombatPlayerReadyRuntime& runtime);
    // IDA 0x57c370.
    [[nodiscard]] static std::uint32_t HandleActorCombatEvent(
        std::uint32_t event, ICombatMonsterReadyRuntime& runtime);
};

}  // namespace pal4
