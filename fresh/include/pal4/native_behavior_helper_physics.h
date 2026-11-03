#pragma once
#include "pal4/behavior_path_endpoint_helper.h"

namespace pal4 {
// Existing GridManager65F1E0/65F100/65F540/65F5A0 composition for endpoint
// and random helpers. Steering/collision supplies the requested vector; this
// integrates it but does not replace those decisions or target writeback.
class NativeBehaviorHelperPhysics final {
public:
    // Slot6C /65F7A0: read-only velocity extrapolation for path correction.
    [[nodiscard]] static BehaviorVector3 PredictPosition(std::uint32_t helper,float elapsed);
    static void Integrate(std::uint32_t helper,const BehaviorVector3& requested,float elapsed);
};
}
