#include "pal4/script_party_magic_query.h"

namespace pal4 {

bool ScriptPartyMagicQuery::AnyMastered(
    IScriptPartyMagicQueryRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
        if (player != 0u && runtime.ReadWord(player + 0xb08u) != 0u &&
            runtime.HasAnyValues(player + 0x938u))
            return true;
    }
    return false;
}

}  // namespace pal4
