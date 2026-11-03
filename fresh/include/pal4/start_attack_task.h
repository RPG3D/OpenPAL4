#pragma once

#include "pal4/script_task_base.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IStartAttackTaskRuntime {
public:
    virtual ~IStartAttackTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
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

class StartAttackTask final {
public:
    static constexpr std::uint32_t kSize = 56u;
    static constexpr std::uint32_t kVtable = 0x845924u;

    // IDA 0x564080/0x564280: construct either attack variant; destroy base.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t parent, IStartAttackTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x564400: schedule FaceTask then an optional RunTask.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, class IStartAttackExecutionRuntime& runtime);
};

class ScriptVector3Operations final {
public:
    // IDA 0x564680/0x5646a0.
    [[nodiscard]] static std::array<float, 3>* Add(
        std::array<float, 3>& destination,
        std::array<float, 3>& operand);
    [[nodiscard]] static std::array<float, 3>* Subtract(
        std::array<float, 3>& destination,
        std::array<float, 3>& operand);
};

struct StartAttackApproachFactorySlot {
    std::uint32_t vtable = 0u;
};

class StartAttackApproachFactory final {
public:
    static constexpr std::uint32_t kPrimaryVtable = 0x845940u;
    static constexpr std::uint32_t kSecondaryVtable = 0x845944u;

    // IDA 0x5646c0/0x5646d0.
    [[nodiscard]] static StartAttackApproachFactorySlot* InitializePrimary(
        StartAttackApproachFactorySlot* slot);
    [[nodiscard]] static StartAttackApproachFactorySlot* InitializeSecondary(
        StartAttackApproachFactorySlot* slot);
};

class IStartAttackExecutionRuntime {
public:
    virtual ~IStartAttackExecutionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) = 0;
    virtual void NormalizeVector(std::array<float, 3>& vector) = 0;
    [[nodiscard]] virtual float ActorClampedChildSize(
        std::uint32_t actor) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool SourceBlocksEvent(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SourceEndsSequence(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string_view GlobalText(
        std::uint32_t global_address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateApproachTask(
        const StartAttackApproachFactorySlot& factory) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::string_view source,
        const std::array<std::uint32_t, 3>& timing) = 0;
    [[nodiscard]] virtual std::uint32_t SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<float, 3>& position) = 0;
};

}  // namespace pal4
