#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPlayerAttributeSyncRuntime {
public:
    virtual ~IScriptPlayerAttributeSyncRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ReadMaxStat(
        std::uint32_t state, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::uint32_t Ftol(double value) = 0;
    [[nodiscard]] virtual std::uint32_t WriteCurrentStat(
        std::uint32_t state, std::uint32_t stat,
        std::uint32_t value) = 0;
};

class ScriptPlayerAttributeSync final {
public:
    // IDA 0x5d4f10, giPlayerSetAttr.
    [[nodiscard]] static std::uint32_t Set(
        std::int32_t player_id, std::int32_t attribute,
        std::int32_t percent, IScriptPlayerAttributeSyncRuntime& runtime);
};

}  // namespace pal4
