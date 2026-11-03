#pragma once

#include "pal4/script_combat_config.h"

#include <cstdint>

namespace pal4 {

class IScriptCombatVipMonsterRuntime : public IScriptCombatConfigRuntime {
public:
    ~IScriptCombatVipMonsterRuntime() override = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
};

class ScriptCombatVipMonster final {
public:
    // IDA 0x5d89a0, giConfigCombatVipMonster.
    [[nodiscard]] static std::uint32_t Configure(
        std::uint32_t monster_id,
        IScriptCombatVipMonsterRuntime& runtime);
};

}  // namespace pal4
