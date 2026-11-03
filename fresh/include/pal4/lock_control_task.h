#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ILockControlTaskRuntime {
public:
    virtual ~ILockControlTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void ResetActorAndSync(std::uint32_t mode) = 0;
    virtual void ProcessUiEvents(std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentActor() = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::string_view state) = 0;
    virtual void SetPlayerState(std::uint32_t value) = 0;
};

class LockControlTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848834u;
    // IDA 0x595fd0/0x595ff0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ILockControlTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ILockControlTaskRuntime& runtime);
};

class UnlockControlTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8489d8u;
    // IDA 0x599ea0/0x599ec0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ILockControlTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ILockControlTaskRuntime& runtime);
};

}  // namespace pal4
