#pragma once
#include "pal4/behavior_path_endpoint_helper.h"

namespace pal4 {
// Borrowed endpoint helper and PathwayGrid images; no ownership or task updates.
// Composes42B940/42BAD0 and the actual path secondary interface at +54.
class NativeBehaviorPathCollision final {
public:
    [[nodiscard]] static BehaviorVector3 Clamped(std::uint32_t helper,float prediction_time);
    [[nodiscard]] static BehaviorVector3 Unclamped(std::uint32_t helper,float prediction_time);
};
}
