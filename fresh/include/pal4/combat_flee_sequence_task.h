#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

struct FleeChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class FleeCandidateChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845d34u;
    // IDA 0x569690.
    [[nodiscard]] static FleeChildFactorySlot* Initialize(
        FleeChildFactorySlot* slot);
};

class FleeFinishChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845d38u;
    // IDA 0x5696a0.
    [[nodiscard]] static FleeChildFactorySlot* Initialize(
        FleeChildFactorySlot* slot);
};

class IFleeSequenceRuntime {
public:
    virtual ~IFleeSequenceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void NotifyActor(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(
        std::uint32_t actor, std::uint32_t virtual_offset) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorValue(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyActorIterator(
        std::uint32_t iterator, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::int32_t ActorPropertySix(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorFlag412(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool CheckCriticalHit(
        float actor_value, float party_average) = 0;
    [[nodiscard]] virtual bool CombatFlag501() = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    [[nodiscard]] virtual std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
};

class FleeSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 48u;
    static constexpr std::uint32_t kVtable = 0x845b68u;

    // IDA 0x567750/0x567770: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);

    // IDA 0x569430: notify actor and schedule flee result children.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IFleeSequenceRuntime& runtime);
    // IDA 0x5696e0: compare actor property 6 to the party average.
    [[nodiscard]] static bool CheckPartyChance(
        std::uint32_t task, IFleeSequenceRuntime& runtime);
};

class QuickFleeSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 48u;
    static constexpr std::uint32_t kVtable = 0x845b84u;

    // IDA 0x567810/0x567830: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);

    // IDA 0x569800: enqueue a single flee finish child.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IFleeSequenceRuntime& runtime);
};

}  // namespace pal4
