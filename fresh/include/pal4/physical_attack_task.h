#pragma once

#include "pal4/script_task_base.h"
#include "pal4/end_attack_task.h"

#include <cstdint>
#include <string_view>
#include <array>

namespace pal4 {

class IPhysicalAttackTaskRuntime {
public:
    virtual ~IPhysicalAttackTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitializeContainer(std::uint32_t container) = 0;
    virtual void DestroyContainer(std::uint32_t container) = 0;
    virtual void AssignName(std::uint32_t name,
                            std::string_view value) = 0;
    virtual void CleanupName(std::uint32_t name) = 0;
};

class PhysicalAttackTask final {
public:
    static constexpr std::uint32_t kSize = 56u;
    static constexpr std::uint32_t kVtable = 0x845948u;

    // IDA 0x5646e0/0x564880: construct a PhysicalAttack task; destroy base.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t parent, IPhysicalAttackTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x564910: initialize and attach the physical attack child.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, class IPhysicalAttackStartRuntime& runtime);
    // IDA 0x564a90: choose wounded, repeated attack, or end attack.
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, class IPhysicalAttackTransitionRuntime& runtime);
    // The existing564A90 constructor, reusable with a different native owner.
    [[nodiscard]] static std::uint32_t BuildContinuation(std::uint32_t parent,bool repeat,
        class IPhysicalAttackContinuationRuntime& runtime);
};

class IPhysicalAttackContinuationRuntime {
public:
    virtual ~IPhysicalAttackContinuationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
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
class IPhysicalAttackTransitionRuntime : public IPhysicalAttackContinuationRuntime {
public:
    [[nodiscard]] virtual bool ActorStateThree(std::uint32_t actor)=0;
    virtual void NotifyStateThree(std::uint32_t actor)=0;
    [[nodiscard]] virtual std::uint32_t ProcessWounded(std::uint32_t actor,std::uint32_t target)=0;
    [[nodiscard]] virtual std::int32_t ActorCounter(std::uint32_t actor,std::uint32_t index)=0;
    virtual void SetActorCounter(std::uint32_t actor,std::uint32_t index,std::int32_t value)=0;
    virtual std::uint32_t CreateContinuation(std::uint32_t parent,bool repeat);
};

struct PhysicalAttackChildFactorySlot {
    std::uint32_t vtable = 0u;
};

class PhysicalAttackChildFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845964u;

    // IDA 0x564a80.
    [[nodiscard]] static PhysicalAttackChildFactorySlot* Initialize(
        PhysicalAttackChildFactorySlot* slot);
};

class IPhysicalAttackStartRuntime {
public:
    virtual ~IPhysicalAttackStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        const PhysicalAttackChildFactorySlot& factory) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t source_text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    // Semantic copy boundary: native owners use host-owned text, bypassing VC6 COW.
    virtual void AssignActionText(std::uint32_t header, std::uint32_t source_header);
    virtual void EraseText(std::uint32_t header,
                           std::uint32_t start,
                           std::uint32_t count) = 0;
    virtual void SetTextErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t header, std::uint32_t length,
        std::uint8_t flag) = 0;
    virtual void CopyText(std::uint32_t header,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
};

}  // namespace pal4
