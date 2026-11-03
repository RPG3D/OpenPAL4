#pragma once

#include <cstdint>

namespace pal4 {

class IScriptFullRageRuntime {
public:
    virtual ~IScriptFullRageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetStat(
        std::uint32_t player_state, std::uint32_t stat,
        std::uint32_t value) = 0;
};

class ScriptFullRage final {
public:
    // IDA 0x5d8b90, giSetFullRage.
    [[nodiscard]] static std::uint32_t Set(
        IScriptFullRageRuntime& runtime);
};

}  // namespace pal4
