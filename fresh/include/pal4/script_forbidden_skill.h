#pragma once

#include <cstdint>

namespace pal4 {

class IScriptForbiddenSkillRuntime {
public:
    virtual ~IScriptForbiddenSkillRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindPlayer(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint8_t SetPresent(
        std::uint32_t collection, std::int32_t skill,
        std::uint32_t enabled) = 0;
};

class ScriptForbiddenSkill final {
public:
    // IDA 0x5d7f90, giPlayerForbidenSkill.
    [[nodiscard]] static std::uint8_t Set(
        std::int32_t player_index, std::int32_t skill,
        std::uint32_t enabled, IScriptForbiddenSkillRuntime& runtime);
};

}  // namespace pal4
