#include "pal4/script_player_rest.h"

namespace pal4 {

std::uint32_t ScriptPlayerRest::RestoreParty(
    IScriptPlayerRestRuntime& runtime) {
    for (std::uint32_t index = 0; index < 4u; ++index) {
        const auto player = runtime.FindPlayer(runtime.PlayerManager(), index);
        if (player == 0u || runtime.ReadWord(player + 0xb08u) == 0u)
            continue;
        const auto bank = player + 0x654u;
        runtime.SetStat(bank, 7u, runtime.GetStat(bank, 0u));
        runtime.SetStat(bank, 9u, runtime.GetStat(bank, 2u));
    }
    return runtime.PlayMusic(runtime.AudioManager(),
                             "gamedata\\PALSound\\WJ063.wav", 1u);
}

}  // namespace pal4
