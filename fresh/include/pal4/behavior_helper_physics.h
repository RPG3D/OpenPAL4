#pragma once
#include "pal4/behavior_path_endpoint_helper.h"
#include "pal4/grid_manager.h"

namespace pal4 {
// Shared composition of the existing 65F100/65F1E0/65F540/65F5A0 algorithms.
// Only endpoint/random helper images are supported. Steering/collision must
// supply the requested vector; this core does not choose a route or bind actors.
class BehaviorHelperPhysics final {
public:
    using Power=double(*)(double,double);
    // Slot6C /65F7A0: const velocity extrapolation for path correction.
    [[nodiscard]] static BehaviorVector3 PredictPosition(const GridManagerState& state,float elapsed);
    // Finite nonnegative elapsed time and finite requested components required.
    // The BA-byte image uses original numeric IDs, never host addresses.
    // Platforms may supply their verified public CRT power implementation.
    static void Integrate(GridManagerState& state,const BehaviorVector3& requested,float elapsed,
                          Power power=nullptr);
};
}
