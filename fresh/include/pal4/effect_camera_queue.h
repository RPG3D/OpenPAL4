#pragma once

#include "pal4/effect_camera_object.h"
#include "pal4/effect_system_object_f_queue.h"

#include <cstdint>

namespace pal4 {

class EffectCameraQueue final {
public:
    // IDA 0x608cd0, facet +40 dispatches to EffectSystem slot +12.
    [[nodiscard]] static std::uint32_t Add(
        const EffectCameraObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
    // IDA 0x608d60, same handle dispatches to slot +16.
    [[nodiscard]] static std::uint32_t Process(
        const EffectCameraObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
};

}  // namespace pal4
