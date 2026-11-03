#include "pal4/script_full_rage.h"

namespace pal4 {

std::uint32_t ScriptFullRage::Set(IScriptFullRageRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.PlayerByIndex(
            runtime.PlayerManager(), index);
        result = player;
        if (runtime.ReadWord(player + 0xb08u) != 0u)
            result = runtime.SetStat(player + 0x654u, 8u, 100u);
    }
    return result;
}

}  // namespace pal4
