#pragma once

#include <cstdint>

namespace pal4 {

class IScriptCombatGroundCameraRuntime {
public:
    virtual ~IScriptCombatGroundCameraRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint8_t UpdateText(
        std::uint32_t combat_system, std::uint32_t text) = 0;
};

class ScriptCombatGroundCamera final {
public:
    // IDA 0x5d8650, giConfigCombatGroundCamera.
    [[nodiscard]] static std::uint8_t Configure(
        std::uint32_t text, IScriptCombatGroundCameraRuntime& runtime);
};

}  // namespace pal4
