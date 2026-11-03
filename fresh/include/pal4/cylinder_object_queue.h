#pragma once

#include "pal4/cylinder_object.h"
#include "pal4/effect_system_object_f_queue.h"

#include <cstdint>

namespace pal4 {

class CylinderObjectQueue final {
public:
    // IDA 0x60df10/0x60dfa0, object +56 facet.
    [[nodiscard]] static std::uint32_t Add(
        const CylinderObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const CylinderObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
};

}  // namespace pal4
