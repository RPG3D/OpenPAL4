#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatSystemUiRuntime {
public:
    virtual ~ICombatSystemUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void ProcessUiEvent(std::string_view name,
                                std::uint32_t enabled,
                                std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t variant) = 0;
    virtual void CleanupHintWindow(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t GetConsole() = 0;
    virtual void UnsubscribeConsoleEvents(std::uint32_t console) = 0;
    [[nodiscard]] virtual std::uint32_t GetActorManager() = 0;
    virtual void DispatchActorEvent36(std::uint32_t collection) = 0;
    virtual void SyncPlayers(){DispatchActorEvent36(GetActorManager()+32u);}
    virtual void EndCombat(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchScriptEvent(
        std::string_view name, std::uint32_t payload) = 0;
};

class CombatSystemUi final {
public:
    // IDA 0x576440.
    [[nodiscard]] static std::uint32_t EndSequence(
        std::uint32_t object, std::uint32_t result,
        ICombatSystemUiRuntime& runtime);
    // IDA 0x5764a0 and 0x5764b0.
    [[nodiscard]] static std::uint32_t ShowWrapper(
        std::uint32_t object, ICombatSystemUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t Show(
        std::uint32_t object, ICombatSystemUiRuntime& runtime);
    // IDA 0x576600 and 0x576610.
    [[nodiscard]] static std::uint32_t CloseWrapper(
        std::uint32_t object, ICombatSystemUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t Close(
        std::uint32_t object, ICombatSystemUiRuntime& runtime);
    // IDA 0x576700 and 0x576720.
    [[nodiscard]] static std::uint32_t ShowEndingWrapper(
        std::uint32_t object, std::uint32_t prize,
        ICombatSystemUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowEnding(
        std::uint32_t object, std::uint32_t prize,
        ICombatSystemUiRuntime& runtime);
    // IDA 0x5767a0 and 0x5767b0.
    [[nodiscard]] static std::uint32_t CloseEndingWrapper(
        std::uint32_t object, ICombatSystemUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloseEnding(
        std::uint32_t object, ICombatSystemUiRuntime& runtime);
};

}  // namespace pal4
