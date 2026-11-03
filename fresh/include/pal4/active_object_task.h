#pragma once

#include <cstdint>

namespace pal4 {

class IActiveObjectTaskRuntime {
public:
    virtual ~IActiveObjectTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t TransientInitialByte() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t GameObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindObjectByName(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void RemoveAnimatedObject(std::uint32_t arena,
                                      std::uint32_t object) = 0;
    virtual void InvokeObjectControl(std::uint32_t object,
                                     std::uint32_t control) = 0;
    virtual void AddAnimatedObject(std::uint32_t arena,
                                   std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t source) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
};

class IActiveObjectTaskSpawnRuntime : public IActiveObjectTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t address) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void NotifyPreviousTask(std::uint32_t task) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class ActiveObjectTask final {
public:
    // IDA 0x593b30/0x593b80/0x593bd0/0x593c50.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IActiveObjectTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IActiveObjectTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IActiveObjectTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t CheckStatus(
        std::uint32_t object, std::int32_t ignored_delta,
        IActiveObjectTaskRuntime& runtime);
    // IDA 0x593c70.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, IActiveObjectTaskSpawnRuntime& runtime);
    // IDA 0x593ea0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t name, std::uint32_t control,
        std::uint32_t context, IActiveObjectTaskRuntime& runtime);
};

}  // namespace pal4
