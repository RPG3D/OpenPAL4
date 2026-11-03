#pragma once

#include <cstdint>

namespace pal4 {

class IScriptTaskReadRuntime {
public:
    virtual ~IScriptTaskReadRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};
class IScriptTaskBaseRuntime : public IScriptTaskReadRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void CleanupObjectCollection(std::uint32_t container) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyContainer(
        std::uint32_t container) = 0;
};

class ICombatTaskActorStatusRuntime {
public:
    virtual ~ICombatTaskActorStatusRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActorStatusVirtual480(
        std::uint32_t actor) = 0;
};

class ScriptTaskBase final {
public:
    static constexpr std::uint32_t kVtable = 0x845390u;

    // IDA 0x563ed0: shared base cleanup for script task objects.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);

    // IDA 0x567800: shared no-op result in several task vtables.
    [[nodiscard]] static std::uint32_t ReturnZero(std::uint32_t argument);

    // IDA 0x5685a0: shared target getter used by combat task vtables.
    [[nodiscard]] static std::uint32_t GetTarget(
        std::uint32_t task, IScriptTaskReadRuntime& runtime);
    // IDA 0x56a000: shared source actor getter in combat task vtables.
    [[nodiscard]] static std::uint32_t GetActor(
        std::uint32_t task, IScriptTaskReadRuntime& runtime);
    // IDA 0x56a3f0: forward to source actor virtual slot +480.
    [[nodiscard]] static std::uint32_t GetActorStatus(
        std::uint32_t task, ICombatTaskActorStatusRuntime& runtime);
};

}  // namespace pal4
