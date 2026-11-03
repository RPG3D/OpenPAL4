#include "pal4/script_player_favor.h"

#include <bit>

namespace pal4 {

std::uint32_t ScriptPlayerFavor::Add(
    const std::int32_t first_index, const std::int32_t second_index,
    const std::int32_t delta, IScriptPlayerFavorRuntime& runtime) {
    const auto first = runtime.PlayerByIndex(
        runtime.PlayerManager(), first_index);
    if (first == 0u) return 0u;
    const auto second = runtime.PlayerByIndex(
        runtime.PlayerManager(), second_index);
    if (second == 0u) return 0u;

    const auto role = runtime.ReadWord(second + 0xa48u);
    if (role < 4u) {
        const auto address = first + 0xa38u + role * 4u;
        const auto sum = runtime.ReadWord(address) +
                         static_cast<std::uint32_t>(delta);
        runtime.WriteWord(address,
                          std::bit_cast<std::int32_t>(sum) < 0 ? 0u : sum);
    }
    return role;
}

}  // namespace pal4
