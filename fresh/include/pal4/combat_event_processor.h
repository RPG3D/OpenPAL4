#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventProcessRuntime {
public:
    virtual ~ICombatEventProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorFromFacet(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActorId(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool HandleActorEvent(std::uint32_t actor) = 0;
    virtual void FinalizeActor(std::uint32_t actor) = 0;
    virtual void GameInstance() = 0;
    virtual void HandleActorProgress(std::uint32_t actor_id) = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorCondition(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint8_t ObjectStatus(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint8_t CheckActorState(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint8_t CheckActorsReady() = 0;
    [[nodiscard]] virtual std::uint8_t IsCombatStateActive(
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    [[nodiscard]] virtual std::uint8_t ProcessScriptEvent(
        std::uint32_t router, std::uint32_t descriptor,
        std::uint32_t payload, std::uint32_t mode) = 0;
};

class CombatEventProcessor final {
public:
    // IDA 0x57b220.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t owner, std::uint32_t event,
        ICombatEventProcessRuntime& runtime);
};

}  // namespace pal4
