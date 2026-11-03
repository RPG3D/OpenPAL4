#include "pal4/combat_reward_record.h"

namespace pal4 {

std::uint32_t CombatRewardRecord::Copy(
    std::uint32_t destination, std::uint32_t source,
    ICombatRewardRecordRuntime& runtime) {
    runtime.WriteByte(destination, runtime.ReadByte(source));
    const auto begin = runtime.ReadWord(source + 4u);
    const auto end = runtime.ReadWord(source + 8u);
    const auto count = begin == 0u ? 0u : (end - begin) / 4u;
    auto target = runtime.Allocate(count * 4u);
    runtime.WriteWord(destination + 4u, target);
    for (auto current = begin; current != end; current += 4u) {
        if (target != 0u)
            runtime.WriteWord(target, runtime.ReadWord(current));
        target += 4u;
    }
    runtime.WriteWord(destination + 8u, target);
    runtime.WriteWord(destination + 12u, target);
    for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    runtime.WriteByte(destination + 36u, runtime.ReadByte(source + 36u));
    return destination;
}

}  // namespace pal4
