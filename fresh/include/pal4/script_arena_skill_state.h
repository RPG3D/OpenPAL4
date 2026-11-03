#pragma once

#include <cstdint>

namespace pal4 {

class IScriptArenaSkillStateRuntime {
public:
    virtual ~IScriptArenaSkillStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaSkillManager() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPlayerData(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t UpdatePlayerData(
        std::uint32_t manager) = 0;
};

class ScriptArenaSkillState final {
public:
    // IDA 0x5d7700, giHideGASkillObject.
    [[nodiscard]] static std::uint32_t HideSkillObject(
        IScriptArenaSkillStateRuntime& runtime);
    // IDA 0x5d7710, giShowSignpost.
    [[nodiscard]] static std::uint32_t ShowSignpost(
        IScriptArenaSkillStateRuntime& runtime);
};

}  // namespace pal4
