#include "pal4/script_party_recovery.h"

namespace pal4 {

std::uint32_t ScriptPartyRecovery::RestoreHp(
    IScriptPartyRecoveryRuntime& runtime) {
    return Restore(0u, 7u, runtime);
}

std::uint32_t ScriptPartyRecovery::RestoreMp(
    IScriptPartyRecoveryRuntime& runtime) {
    return Restore(2u, 9u, runtime);
}

std::uint32_t ScriptPartyRecovery::Restore(
    std::uint32_t max_stat, std::uint32_t current_stat,
    IScriptPartyRecoveryRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index != 4u; ++index) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
        result = player;
        if (runtime.ReadWord(player + 0xb08u) == 0u) continue;
        const auto stats = player + 0x654u;
        const auto value = runtime.ReadMaxStat(stats, max_stat);
        result = runtime.WriteCurrentStat(stats, current_stat, value);
    }
    return result;
}

}  // namespace pal4
