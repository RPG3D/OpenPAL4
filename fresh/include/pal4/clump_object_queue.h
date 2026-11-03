#pragma once

#include "pal4/clump_object.h"
#include "pal4/effect_system_object_f_queue.h"

#include <cstdint>

namespace pal4 {

class ClumpObjectQueue final {
public:
    // IDA 0x60bb80, object +64 facet, EffectSystem slot +12.
    [[nodiscard]] static std::uint32_t Add(
        const ClumpObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
    // IDA 0x60bc10, same facet, EffectSystem slot +16.
    [[nodiscard]] static std::uint32_t Process(
        const ClumpObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
};

}  // namespace pal4
