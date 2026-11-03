#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEndingWindowInputRuntime {
public:
    virtual ~ICombatEndingWindowInputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t EndCombatSequence(
        std::uint32_t system, std::uint32_t prior_mode) = 0;
    virtual void ShowResult(std::uint32_t window) = 0;
    virtual void HideResult(std::uint32_t window) = 0;
};

class CombatEndingWindowInput final {
public:
    // IDA 0x544db0.
    [[nodiscard]] static std::uint32_t FinishCombat(
        std::uint32_t window, ICombatEndingWindowInputRuntime& runtime);
    // IDA 0x544de0; navigation is window + 0x28.
    [[nodiscard]] static std::uint8_t HandleInput(
        std::uint32_t navigation, std::uint32_t event,
        ICombatEndingWindowInputRuntime& runtime);
    // IDA 0x544e40.
    [[nodiscard]] static std::uint8_t HandleMouseDown(
        std::uint32_t window, std::uint32_t event,
        ICombatEndingWindowInputRuntime& runtime);
};

}  // namespace pal4
