#pragma once

#include <cstdint>

namespace pal4 {

class IScriptCurrentPlayerVisibilityRuntime {
public:
    virtual ~IScriptCurrentPlayerVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptOnRemove(
        std::uint32_t arena, std::uint32_t player) = 0;
    virtual void SetStateFlag(std::uint32_t player, std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptOnAdd(
        std::uint32_t arena, std::uint32_t player) = 0;
};

class ScriptCurrentPlayerVisibility final {
public:
    // IDA 0x5d5460, giPlayerCurrentSetVisible.
    [[nodiscard]] static std::uint32_t Set(
        std::uint8_t visible, IScriptCurrentPlayerVisibilityRuntime& runtime);
};

}  // namespace pal4
