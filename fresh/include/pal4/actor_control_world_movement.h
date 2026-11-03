#pragma once

#include "pal4/actor_control_discrete_movement.h"

namespace pal4 {

class ActorControlWorldMovement final {
public:
    [[nodiscard]] static std::uint32_t ApplyDelta(
        std::uint32_t actor, std::uint32_t delta_address,
        ActorControlMovementGlobals& globals,
        IActorControlDiscreteMovementRuntime& runtime);
};

}  // namespace pal4
