#include "pal4/script_player_attribute_sync.h"

#include <bit>

namespace pal4 {
namespace {

constexpr float kPercentScale = std::bit_cast<float>(0x3c23d70au);

std::uint32_t UpdateOne(std::uint32_t player, std::int32_t attribute,
                        std::int32_t percent,
                        IScriptPlayerAttributeSyncRuntime& runtime) {
    if (attribute != -1 && attribute != 0 && attribute != 1) return player;
    const auto state = player + 0x654u;
    const auto scaled_percent = static_cast<double>(percent) *
                                static_cast<double>(kPercentScale);

    if (attribute == -1) {
        const auto rounded_percent = static_cast<float>(scaled_percent);
        const auto hp = runtime.ReadMaxStat(state, 0u);
        const auto hp_value = runtime.Ftol(
            static_cast<double>(rounded_percent) * static_cast<double>(hp));
        (void)runtime.WriteCurrentStat(state, 7u, hp_value);
        const auto mp = runtime.ReadMaxStat(state, 2u);
        const auto mp_value = runtime.Ftol(
            static_cast<double>(rounded_percent) * static_cast<double>(mp));
        return runtime.WriteCurrentStat(state, 9u, mp_value);
    }

    const auto max_stat = attribute == 0 ? 0u : 2u;
    const auto current_stat = attribute == 0 ? 7u : 9u;
    const auto maximum = runtime.ReadMaxStat(state, max_stat);
    const auto value = runtime.Ftol(
        scaled_percent * static_cast<double>(maximum));
    return runtime.WriteCurrentStat(state, current_stat, value);
}

}  // namespace

std::uint32_t ScriptPlayerAttributeSync::Set(
    std::int32_t player_id, std::int32_t attribute,
    std::int32_t percent, IScriptPlayerAttributeSyncRuntime& runtime) {
    if (player_id >= 0) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                                  player_id);
        return player == 0u ? 0u :
               UpdateOne(player, attribute, percent, runtime);
    }

    std::uint32_t result = 0u;
    for (std::int32_t index = 0; index < 4; ++index) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                                  index);
        if (player == 0u) return 0u;
        result = UpdateOne(player, attribute, percent, runtime);
    }
    return result;
}

}  // namespace pal4
