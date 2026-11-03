#include "pal4/game_object_state_float_gate.h"

#include <bit>

namespace pal4 {

bool GameObjectStateFloatGate::Check(
    const BehaviorPathEndpointState& helper) {
    return std::bit_cast<float>(helper.words[0xc4u / 4u]) >=
           std::bit_cast<float>(helper.words[0xc0u / 4u]);
}

}  // namespace pal4
