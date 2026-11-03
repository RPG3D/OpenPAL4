#pragma once

#include "pal4/script_actor_event_processor.h"
#include "pal4/script_task_base.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct QuickAttackChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class QuickAttackChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845988u;

    // IDA 0x565260.
    [[nodiscard]] static QuickAttackChildFactorySlot* Initialize(
        QuickAttackChildFactorySlot* slot);
};

class IQuickAttackStartRuntime : public IScriptActorEventRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetSelectedTarget(std::uint32_t selected,
                                   std::uint32_t source,
                                   std::uint32_t target) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        const QuickAttackChildFactorySlot& factory) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t source_text,
        const std::array<std::uint32_t, 3>& timing) = 0;
};

class QuickAttackTask final {
public:
    static constexpr std::uint32_t kSize = 56u;
    static constexpr std::uint32_t kVtable = 0x8459a8u;
    static constexpr std::uint32_t kEndVtable = 0x84598cu;

    // IDA 0x5651b0: select another target and schedule the quick-attack child.
    [[nodiscard]] static bool Start(std::uint32_t task,
                                    IQuickAttackStartRuntime& runtime);
    // IDA 0x565270: continue the quick attack or create cbQuickEnd.
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, class IQuickAttackTransitionRuntime& runtime);
    // IDA 0x5654e0: release the shared script task base.
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
};

class QuickAttackEndTask final {
public:
    static constexpr std::uint32_t kVtable = QuickAttackTask::kEndVtable;

    // IDA 0x565570/0x565590: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
};

class IQuickAttackTransitionRuntime {
public:
    virtual ~IQuickAttackTransitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual bool CombatActive() = 0;
    [[nodiscard]] virtual bool SourceStateThree(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessDead(
        std::uint32_t source, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t EndAction(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::int32_t ActorCounter(
        std::uint32_t actor, std::uint32_t index) = 0;
    virtual void SetActorCounter(std::uint32_t actor,
                                 std::uint32_t index,
                                 std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitializeContainer(std::uint32_t container) = 0;
    virtual void DestroyContainer(std::uint32_t container) = 0;
    virtual void SetNameErrorFlag(std::uint32_t name,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateNameBuffer(
        std::uint32_t name, std::uint32_t length,
        std::uint8_t flag) = 0;
    virtual void CopyName(std::uint32_t name,
                          std::string_view value) = 0;
    virtual void CleanupName(std::uint32_t name) = 0;
};

}  // namespace pal4
