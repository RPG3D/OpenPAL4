#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPartyRecoveryRuntime {
public:
    virtual ~IScriptPartyRecoveryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadMaxStat(
        std::uint32_t stats, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::uint32_t WriteCurrentStat(
        std::uint32_t stats, std::uint32_t stat,
        std::uint32_t value) = 0;
};

class ScriptPartyRecovery final {
public:
    // IDA 0x5d54c0, giSetFullHP.
    [[nodiscard]] static std::uint32_t RestoreHp(
        IScriptPartyRecoveryRuntime& runtime);
    // IDA 0x5d5510, giSetFullMP.
    [[nodiscard]] static std::uint32_t RestoreMp(
        IScriptPartyRecoveryRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Restore(
        std::uint32_t max_stat, std::uint32_t current_stat,
        IScriptPartyRecoveryRuntime& runtime);
};

}  // namespace pal4
