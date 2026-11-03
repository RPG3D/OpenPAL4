#include "pal4/game_arena_record_compactor.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t GameArenaRecordCompactor::Compact(
    const std::uint32_t range, const std::uint32_t destination,
    const std::uint32_t source, IGameArenaRecordMemory& memory) {
    const auto end = memory.ReadWord(range + 8u);
    if (source > end || (end - source) % 12u != 0u)
        throw std::invalid_argument("invalid 12-byte arena record range");

    auto next_source = source;
    auto next_destination = destination;
    while (next_source != end) {
        for (std::uint32_t offset = 0u; offset < 12u; offset += 4u)
            memory.WriteWord(next_destination + offset,
                             memory.ReadWord(next_source + offset));
        next_source += 12u;
        next_destination += 12u;
    }
    memory.WriteWord(range + 8u, next_destination);
    return destination;
}

}  // namespace pal4
