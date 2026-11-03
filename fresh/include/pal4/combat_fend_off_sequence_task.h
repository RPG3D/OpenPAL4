#pragma once

#include "pal4/combat_action_factory.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

struct FendOffImpactChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class FendOffImpactChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845d10u;

    // IDA 0x568f60: initialize the first FendOff child factory slot.
    [[nodiscard]] static FendOffImpactChildFactorySlot* Initialize(
        FendOffImpactChildFactorySlot* slot);
};

class IFendOffStartRuntime {
public:
    virtual ~IFendOffStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual bool ActorCheckStateThree(
        std::uint32_t actor) = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::uint32_t state,
                               std::uint32_t enabled) = 0;
    virtual void ActorVirtual396(std::uint32_t actor,
                                 std::uint32_t argument) = 0;
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

class IFendOffDeathFactoryRuntime
    : public ICombatActionFactoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ActorCheckStateThree(
        std::uint32_t actor) = 0;
};

class FendOffSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845c2cu;

    // IDA 0x5681c0/0x5681e0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);

    // IDA 0x568e30: change target state and schedule three children.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IFendOffStartRuntime& runtime);

    // IDA 0x568f70: create Dead with source and target exchanged.
    [[nodiscard]] static std::uint32_t CreateDeathTask(
        std::uint32_t task, IFendOffDeathFactoryRuntime& runtime);
};

class QuickFendOffSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845c48u;

    // IDA 0x568270/0x568290: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);

    // IDA 0x5690e0: create QuickDead with exchanged actors.
    [[nodiscard]] static std::uint32_t CreateDeathTask(
        std::uint32_t task, IFendOffDeathFactoryRuntime& runtime);
};

}  // namespace pal4
