#pragma once
#include "pal4/native_behavior_movement.h"

namespace pal4 {
using NativeBehaviorRandomMovementStats=NativeBehaviorMovementStats;
using NativeBehaviorRandomMovementResult=NativeBehaviorMovementResult;
// Original class3 helper42CA10/42CA80 composition. Obstacles are the original
// secondary-context sphere-record pointers (radius+4, position+8); neighbors
// come from the helper's actual spatial association. The shared random stream
// must be the same one used by helper construction/other behavior services.
class NativeBehaviorRandomMovement final : public NativeBehaviorMovement {
public:
    using NativeBehaviorMovement::NativeBehaviorMovement;
    NativeBehaviorRandomMovementResult Update(std::uint32_t helper,
        std::span<const std::uint32_t> obstacles,float elapsed) {
        return UpdateRandom(helper,obstacles,elapsed);
    }
};
}
