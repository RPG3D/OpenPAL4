#pragma once

#include "pal4/script_task_base.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct EndAttackBaseFactorySlot {
    std::uint32_t vtable = 0u;
};

class EndAttackBaseFactory final {
public:
    static constexpr std::uint32_t kVtable = 0x845984u;

    // IDA 0x564f40: initialize the first child factory wrapper.
    [[nodiscard]] static EndAttackBaseFactorySlot* Initialize(
        EndAttackBaseFactorySlot* slot);
};

class IEndAttackStartRuntime {
public:
    virtual ~IEndAttackStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::string_view GlobalText(
        std::uint32_t global_address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::string_view source,
        const std::array<std::uint32_t, 3>& timing) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SourceEndsSequence(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<float, 3>& position) = 0;
};

class EndAttackTask final {
public:
    static constexpr std::uint32_t kSize = 56u;
    static constexpr std::uint32_t kVtable = 0x845968u;

    // IDA 0x564cf0/0x564d80: base destruction and task scheduling.
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IEndAttackStartRuntime& runtime);
    // IDA 0x564f50: emit select_join_atk and optionally create StartAttack.
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, class IEndAttackTransitionRuntime& runtime);
};

class IEndAttackTransitionRuntime {
public:
    virtual ~IEndAttackTransitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool AllCombatObjectsReady(
        std::uint32_t combat) = 0;
    virtual void DispatchEvent(std::uint32_t key,
                               std::uint32_t first,
                               std::uint32_t second,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ActorChance(std::uint32_t actor,
                                            std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t SourceStatus(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t TargetStatus(
        std::uint32_t actor) = 0;
    virtual void TargetSetSource(std::uint32_t target,
                                 std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitializeContainer(std::uint32_t container) = 0;
    virtual void DestroyContainer(std::uint32_t container) = 0;
    virtual void AssignName(std::uint32_t name,
                            std::string_view value) = 0;
    virtual void CleanupName(std::uint32_t name) = 0;
};

}  // namespace pal4
