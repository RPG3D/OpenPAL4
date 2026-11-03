#pragma once

#include <cstdint>

namespace pal4 {

class ICombatResultMoveRuntime {
public:
    virtual ~ICombatResultMoveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual float GetYPosition(std::uint32_t window) = 0;
    virtual void SetYPosition(std::uint32_t window, float value) = 0;
};

class CombatResultMove final {
public:
    // IDA CombatResultMove_Update at 0x548b10.
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t element, float elapsed,
        ICombatResultMoveRuntime& runtime);

    // IDA CombatResultMove_Reached at 0x548b70.
    [[nodiscard]] static bool Reached(
        std::uint32_t element, ICombatResultMoveRuntime& runtime);
};

}  // namespace pal4
