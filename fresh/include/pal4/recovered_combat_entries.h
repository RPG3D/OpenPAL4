#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMatFxScriptBootRuntime {
public:
    virtual ~IMatFxScriptBootRuntime() = default;
    virtual void PrepareScripts() = 0;
    virtual void ExecuteScript(std::string_view path) = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t SetupCombatCamera(
        std::uint32_t owner) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class MatFxScriptBoot final {
public:
    // IDA 0x53f550, including the noncontiguous return tail at 0x53ba20.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t owner, IMatFxScriptBootRuntime& runtime);
};

class IActorManagerFloatTaskRuntime {
public:
    virtual ~IActorManagerFloatTaskRuntime() = default;
    [[nodiscard]] virtual int Process(std::uint32_t queue, float value) = 0;
};

class ActorManagerFloatTask final {
public:
    // IDA 0x543f60.
    [[nodiscard]] static int ConvertAndProcess(
        std::uint32_t manager, double value,
        IActorManagerFloatTaskRuntime& runtime);
};

class ICombatTaskResultWordRuntime {
public:
    virtual ~ICombatTaskResultWordRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address)=0;
};
class ICombatEndingScoreElementRuntime : public ICombatTaskResultWordRuntime {
public:
    virtual ~ICombatEndingScoreElementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CleanupCollection(std::uint32_t collection) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual int DestroyContainer(std::uint32_t container) = 0;
};

class CombatEndingScoreElement final {
public:
    // IDA 0x547a90 and 0x547ac0.
    [[nodiscard]] static std::uint32_t ResultWord(
        std::uint32_t element, ICombatTaskResultWordRuntime& runtime);
    [[nodiscard]] static int Destroy(
        std::uint32_t element, ICombatEndingScoreElementRuntime& runtime);
};

class ICombatConsoleInitThunkRuntime {
public:
    virtual ~ICombatConsoleInitThunkRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeCombatConsole() = 0;
};

class CombatConsoleInitThunk final {
public:
    // IDA 0x5484c0.
    [[nodiscard]] static std::uint32_t Call(ICombatConsoleInitThunkRuntime& runtime);
};

}  // namespace pal4
