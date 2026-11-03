#pragma once

#include "pal4/dynamic_strip_object.h"
#include "pal4/effect_system_object_f_queue.h"

#include <cstdint>

namespace pal4 {

class DynamicStripObjectQueue final {
public:
    // IDA 0x60f830/0x60f8c0; object +48 facet.
    [[nodiscard]] static std::uint32_t Add(
        const DynamicStripObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const DynamicStripObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
};

}  // namespace pal4
