#pragma once

#include "pal4/game_string_reference.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICombatConsoleHintActorRuntime {
public:
    virtual ~ICombatConsoleHintActorRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address)=0;
};
class ICombatConsoleHintRuntime : public IGameStringReferenceRuntime,
                                  public ICombatConsoleHintActorRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t UiManager() = 0;
    [[nodiscard]] virtual std::uint32_t HintWindow(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t CombatHintWindow2(
        std::uint32_t manager) = 0;
    virtual void CloseHint(std::uint32_t window) = 0;
    virtual void CleanupCombatHint(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t ShowHint(
        std::uint32_t window, std::uint32_t text,
        const std::array<std::uint32_t, 2>& position,
        std::uint32_t duration_bits, std::uint32_t first_flag,
        std::uint32_t second_flag) = 0;
    [[nodiscard]] virtual std::uint32_t ShowCombatHint(
        std::uint32_t window, std::uint32_t text,
        const std::array<std::uint32_t, 2>& position,
        std::uint32_t duration_bits, std::uint32_t first_flag,
        std::uint32_t second_flag) = 0;
};

class CombatConsoleHintEvents final {
public:
    // Original540170/5401F0 early type gate, before any hint/DB service.
    [[nodiscard]] static bool ShouldShowConditional(std::uint32_t actor,ICombatConsoleHintActorRuntime& runtime) {
        return runtime.ReadWord(actor+804u)==2u;
    }
    // IDA 0x540100, 0x540170 and 0x5401f0.
    [[nodiscard]] static std::uint32_t ShowDefault(
        std::uint32_t argument, ICombatConsoleHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowFleeingMonsterHint(
        std::uint32_t argument, ICombatConsoleHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowFleeFailedMonsterHint(
        std::uint32_t argument, ICombatConsoleHintRuntime& runtime);
    // IDA 0x540360, 0x5403d0 and 0x540440.
    [[nodiscard]] static std::uint32_t ShowEventTextAt16(
        std::uint32_t event, ICombatConsoleHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowEventTextAt20(
        std::uint32_t event, ICombatConsoleHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowEventTextAt12(
        std::uint32_t event, ICombatConsoleHintRuntime& runtime);
    // IDA 0x540d20.
    [[nodiscard]] static std::uint32_t ShowKey10c(
        std::uint32_t argument, ICombatConsoleHintRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t ShowConditional(
        std::uint32_t argument, std::uint32_t string_key,
        ICombatConsoleHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t GameText(
        std::uint32_t string_key, ICombatConsoleHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowEventText(
        std::uint32_t event, std::uint32_t text_offset,
        ICombatConsoleHintRuntime& runtime);
};

}  // namespace pal4
