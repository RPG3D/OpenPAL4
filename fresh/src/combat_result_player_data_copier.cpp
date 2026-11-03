#include "pal4/combat_result_player_data_copier.h"

namespace pal4 {
namespace {

[[nodiscard]] std::uint32_t Count(std::uint32_t begin,
                                  std::uint32_t end) {
    return begin == 0u ? 0u : (end - begin) >> 2u;
}

void CopyWords(std::uint32_t destination, std::uint32_t source,
               std::uint32_t count,
               ICombatResultPlayerDataRuntime& runtime) {
    for (std::uint32_t i = 0u; i < count; ++i) {
        const auto value = runtime.ReadWord(source + 4u * i);
        if (destination != 0u)
            runtime.WriteWord(destination + 4u * i, value);
    }
}

}  // namespace

std::uint32_t CombatResultPlayerDataCopier::Copy(
    std::uint32_t destination, std::uint32_t source,
    ICombatResultPlayerDataRuntime& runtime) {
    if (destination != source) {
        const auto source_begin = runtime.ReadWord(source + 4u);
        const auto source_end = runtime.ReadWord(source + 8u);
        const auto count = Count(source_begin, source_end);
        auto target_begin = runtime.ReadWord(destination + 4u);
        const auto target_end = runtime.ReadWord(destination + 8u);
        const auto target_capacity_end = runtime.ReadWord(destination + 12u);
        const auto size = Count(target_begin, target_end);
        const auto capacity = Count(target_begin, target_capacity_end);

        if (count <= size) {
            CopyWords(target_begin, source_begin, count, runtime);
            runtime.WriteWord(destination + 8u, target_begin + 4u * count);
        } else if (count <= capacity) {
            CopyWords(target_begin, source_begin, size, runtime);
            CopyWords(target_end, source_begin + 4u * size,
                      count - size, runtime);
            runtime.WriteWord(destination + 8u, target_begin + 4u * count);
        } else {
            runtime.Free(target_begin);
            const auto allocation = runtime.Allocate(4u * count);
            runtime.WriteWord(destination + 4u, allocation);
            CopyWords(allocation, source_begin, count, runtime);
            target_begin = allocation + 4u * count;
            runtime.WriteWord(destination + 8u, target_begin);
            runtime.WriteWord(destination + 12u, target_begin);
        }
    }

    for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.WriteByte(destination + 36u, runtime.ReadByte(source + 36u));
    return destination;
}

}  // namespace pal4
