#pragma once

#include "pal4/combat_action_factory.h"
#include "pal4/combat_ready_target_scan.h"
#include "pal4/use_property_task.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

class IUsePropertySequenceStartRuntime : public IUsePropertyTargetRuntime {
public:
    virtual ~IUsePropertySequenceStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> ActorChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> ActorTargetPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyMovementMode(
        std::uint32_t property) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    virtual void SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<std::uint32_t, 3>& position) = 0;
    [[nodiscard]] virtual std::uint32_t AssignChildText(
        std::uint32_t header, std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t SetThrowPropertyTargets(
        std::uint32_t embedded, std::uint32_t source,
        std::uint32_t target, std::uint32_t property) = 0;
};

class IUsePropertyTransitionRuntime : public ICombatActionFactoryRuntime {
public:
    // Native domains may supply the same Dead -> EndAction chain using their
    // existing typed owners, rather than allocate a second private death owner.
    virtual bool TryCreateDeadWithEndAction(std::uint32_t,std::uint32_t,std::uint32_t&) { return false; }
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool SourceStateThree(
        std::uint32_t source) = 0;
};

class IUsePropertyActionRuntime : public ICombatReadyTargetRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool PropertyExists(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual bool ActorPropertySlot8(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t UsePropertyAction(
        std::uint32_t property, std::uint32_t source,
        std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t NormalAction(
        std::uint32_t source, std::uint32_t target,
        std::uint32_t mode) = 0;
};

class IQuickUsePropertyStartRuntime {
public:
    virtual ~IQuickUsePropertyStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void NotifySource(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
};

class IQuickUsePropertyTransitionRuntime : public IUsePropertyTransitionRuntime,
                                           public ICombatReadyTargetRuntime {
public:
    [[nodiscard]] virtual bool CombatActive() = 0;
    virtual void NotifyUnreadyActor(std::uint32_t actor,
                                    std::uint32_t source) = 0;
};

struct QuickUsePropertyChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class QuickUsePropertyChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845dc0u;
    // IDA 0x56ac50.
    [[nodiscard]] static QuickUsePropertyChildFactorySlot* Initialize(
        QuickUsePropertyChildFactorySlot* slot);
};

struct UsePropertyChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class UsePropertyChildFactories final {
public:
    static constexpr std::uint32_t kCastVtable = 0x845db4u;
    static constexpr std::uint32_t kThrowVtable = 0x845db8u;
    static constexpr std::uint32_t kDirectVtable = 0x845dbcu;

    // IDA 0x56a990/0x56a9a0/0x56a9b0.
    [[nodiscard]] static UsePropertyChildFactorySlot* InitializeCast(
        UsePropertyChildFactorySlot* slot);
    [[nodiscard]] static UsePropertyChildFactorySlot* InitializeThrow(
        UsePropertyChildFactorySlot* slot);
    [[nodiscard]] static UsePropertyChildFactorySlot* InitializeDirect(
        UsePropertyChildFactorySlot* slot);
};

class UsePropertySequenceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845b14u;

    // IDA 0x567250/0x567270: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56a710/0x56a9c0.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IUsePropertySequenceStartRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, IUsePropertyTransitionRuntime& runtime);
    // IDA 0x56aa10.
    [[nodiscard]] static std::uint32_t CreateAction(
        std::uint32_t task, std::uint32_t source,
        IUsePropertyActionRuntime& runtime);
};

class QuickUsePropertySequenceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845b30u;

    // IDA 0x567300/0x567320: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56abd0/0x56ac60.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IQuickUsePropertyStartRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, IQuickUsePropertyTransitionRuntime& runtime);
};

}  // namespace pal4
