#pragma once

#include "pal4/pal_actor_control_interaction.h"
#include "pal4/pal_actor_control_yaw_input.h"

namespace pal4 {

class InputManager;

// Reads the three distinct input-record groups used by IDA 0x424750,
// 0x424fc0 and 0x425430 from the owned InputManager image.
class ActorControlInputSnapshot final {
public:
    [[nodiscard]] static ActorControlMouseInput Yaw(
        const InputManager& input);
    [[nodiscard]] static ActorControlMouseInput Drag(
        const InputManager& input);
    [[nodiscard]] static ActorInteractionInput Interaction(
        const InputManager& input);
};

}  // namespace pal4
