#pragma once

#include <cstdint>

namespace pal4 {

class ITouchOtherTaskRuntime {
public:
    virtual ~ITouchOtherTaskRuntime() = default;
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
    virtual void InitializeEmptyString(std::uint32_t record) = 0;
    virtual void CleanupString(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeString(
        std::uint32_t record, std::uint32_t source,
        std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t FindObjectByName(
        std::uint32_t name) = 0;
    virtual void TouchObject(std::uint32_t target,
                            std::uint32_t owner_position) = 0;
    [[nodiscard]] virtual bool ObjectDone(std::uint32_t target) = 0;
};

class ITouchOtherTaskSpawnRuntime : public virtual ITouchOtherTaskRuntime {
public:
    virtual std::uint32_t AllocateMemory(std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual std::uint32_t ConstructHelperTask(std::uint32_t memory) = 0;
    virtual void AddHelperTask(std::uint32_t context, std::uint32_t helper,
        std::uint32_t mode, std::uint32_t wait, std::uint32_t duration) = 0;
    virtual void SetHelperOwner(std::uint32_t helper, std::uint32_t owner) = 0;
    virtual void NotifyPreviousTask(std::uint32_t previous) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class TouchOtherTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8489b4u;
    // IDA 0x599ca0..0x599e30.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ITouchOtherTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ITouchOtherTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ITouchOtherTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, ITouchOtherTaskRuntime& runtime);
    // IDA599120: delegate to the previous task, or append a stored-transform helper.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, ITouchOtherTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t object_name, std::uint32_t wait,
        std::uint32_t context, ITouchOtherTaskRuntime& runtime);
};

}  // namespace pal4
