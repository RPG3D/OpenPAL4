#include "pal4/actor_effect_record.h"

namespace pal4 {

std::uint32_t ActorEffectRecord::Copy(
    std::uint32_t destination, std::uint32_t source,
    IActorEffectRecordRuntime& runtime) {
    if (destination == 0u) return 0u;
    for (std::uint32_t offset = 0u; offset < 12u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    return destination;
}

std::uint32_t ActorEffectRecord::Destroy(
    std::uint32_t record, std::uint8_t flags,
    IActorEffectRecordRuntime& runtime) {
    runtime.ResetHandle(record);
    if ((flags & 1u) != 0u) runtime.Free(record);
    return record;
}

}  // namespace pal4
