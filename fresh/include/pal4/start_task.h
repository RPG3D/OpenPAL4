#pragma once

#include "pal4/move_task.h"

#include <cstdint>

namespace pal4 {

class IStartTaskRuntime {
public:
    virtual ~IStartTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetOwnerPosition(std::uint32_t owner,
                                  const MoveTaskVector& position,
                                  std::uint32_t mode) = 0;
    virtual void SetOwnerRotation(std::uint32_t owner,
                                  const MoveTaskVector& rotation,
                                  std::uint32_t mode) = 0;
};

class IStartTaskSpawnRuntime : public virtual IStartTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t size) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void AddReverseTask(std::uint32_t context,
                                std::uint32_t task,
                                std::uint32_t timing_address) = 0;
    virtual void InsertOwnerTask(std::uint32_t collection,
                                 std::uint32_t task) = 0;
    virtual void NotifyPreviousTask(std::uint32_t previous) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t memory) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class StartTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x848988u;
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, IStartTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IStartTaskSpawnRuntime& runtime);
};

class StartTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848964u;
    // IDA 0x599470..0x599810.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IStartTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IStartTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IStartTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, IStartTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, IStartTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        const MoveTaskVector& position,
        const MoveTaskVector& rotation,
        std::uint32_t context, IStartTaskRuntime& runtime);
};

}  // namespace pal4
