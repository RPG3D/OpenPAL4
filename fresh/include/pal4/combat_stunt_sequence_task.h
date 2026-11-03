#pragma once

#include "pal4/combat_child_text_assignment.h"
#include "pal4/combat_action_factory.h"
#include "pal4/combat_ready_target_scan.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;
class IEndStuntStartRuntime;

class EndStuntSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 56u;
    static constexpr std::uint32_t kVtable = 0x845d78u;

    // IDA 0x56a010/0x56a030: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56a270: schedule post-Stunt run or position children.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IEndStuntStartRuntime& runtime);
};

struct StuntActorPosition {
    std::uint32_t address = 0u;
    std::array<float, 3> values{};
};

class IEndStuntStartRuntime {
public:
    virtual ~IEndStuntStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    [[nodiscard]] virtual StuntActorPosition ActorPosition(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const StuntActorPosition& position) = 0;
};

class IStuntTransitionRuntime : public ICombatActionFactoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool SourceStateThree(
        std::uint32_t source) = 0;
    virtual std::uint32_t CreateDeadAction(std::uint32_t source,std::uint32_t target) {
        return CombatDeadActionFactory::Create(source,target,*this);
    }
    virtual void AttachDeadSuccessor(std::uint32_t dead,std::uint32_t next) {
        WriteWord(dead+36u,next);
    }
};

class IStuntActionRuntime : public ICombatReadyTargetRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CanUseStunt(
        std::uint32_t source, std::uint32_t stunt) = 0;
    [[nodiscard]] virtual bool ActorPropertySlot8(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t StuntAction(
        std::uint32_t stunt, std::uint32_t source,
        std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t NormalAction(
        std::uint32_t source, std::uint32_t target,
        std::uint32_t mode) = 0;
};

struct QuickStuntChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class QuickStuntChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845d94u;
    // IDA 0x56a480.
    [[nodiscard]] static QuickStuntChildFactorySlot* Initialize(
        QuickStuntChildFactorySlot* slot);
};

class IQuickStuntStartRuntime {
public:
    virtual ~IQuickStuntStartRuntime() = default;
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

class IQuickStuntTransitionRuntime : public IStuntTransitionRuntime,
                                      public ICombatReadyTargetRuntime {
public:
    [[nodiscard]] virtual bool CombatActive() = 0;
    virtual void NotifyUnreadyActor(std::uint32_t actor,
                                    std::uint32_t source) = 0;
};

struct StuntChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class StuntChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845d74u;
    // IDA 0x569e50.
    [[nodiscard]] static StuntChildFactorySlot* Initialize(
        StuntChildFactorySlot* slot);
};

class IStuntSequenceRuntime : public ICombatChildTextRuntime {
public:
    virtual ~IStuntSequenceRuntime() = default;
    [[nodiscard]] virtual std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) = 0;
    virtual void NormalizeVector(std::array<float, 3>& vector) = 0;
    [[nodiscard]] virtual float ActorClampedChildSize(
        std::uint32_t actor) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t StuntMovementMode(
        std::uint32_t stunt) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    virtual void SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<float, 3>& position) = 0;
    [[nodiscard]] virtual std::uint32_t ActorStuntText(
        std::uint32_t source, std::uint32_t stunt) = 0;
};

class StuntSequenceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845adcu;

    // IDA 0x566f00/0x566f20: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x5699d0: schedule movement, optional position and Stunt child.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, IStuntSequenceRuntime& runtime);
    // IDA 0x569e60: choose Dead plus EndAction or construct EndStunt.
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, IStuntTransitionRuntime& runtime);
    // IDA 0x56a0c0: choose Stunt or ordinary attack after target search.
    [[nodiscard]] static std::uint32_t CreateAction(
        std::uint32_t task, std::uint32_t source,
        IStuntActionRuntime& runtime);
};

class QuickStuntSequenceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845af8u;

    // IDA 0x566fb0/0x566fd0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56a400/0x56a490: child start and follow-up action.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IQuickStuntStartRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, IQuickStuntTransitionRuntime& runtime);
};

}  // namespace pal4
