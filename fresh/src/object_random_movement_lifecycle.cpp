#include "pal4/object_random_movement_lifecycle.h"

#include <bit>
#include <cmath>

namespace pal4 {

void ObjectRandomMovementRuntimeStub::ConstructBase(
    ObjectRandomMovementState&) {}
void ObjectRandomMovementRuntimeStub::CleanupBase(
    ObjectRandomMovementState&) {}
void ObjectRandomMovementRuntimeStub::Free(std::uint32_t) {}
BehaviorVector3 ObjectRandomMovementRuntimeStub::RandomVector() { return {}; }

ObjectRandomMovementState& ObjectRandomMovementLifecycle::Construct(
    ObjectRandomMovementState& state,
    IObjectRandomMovementRuntime& runtime) {
    runtime.ConstructBase(state);
    for (std::uint32_t index = 50u; index < 56u; ++index)
        state.words[index] = 0u;
    state.words[47] = 0u;
    state.words[49] = 0u;
    state.words[0] = kVtable;
    state.words[48] = 0x40000000u;

    try {
        const auto random = runtime.RandomVector();
        const auto length = std::sqrt(random[0] * random[0] +
                                      random[2] * random[2]);
        if (length > 0.0f) {
            const auto scale = 1.0f / length;
            state.words[50] = std::bit_cast<std::uint32_t>(random[0] * scale);
            state.words[52] = std::bit_cast<std::uint32_t>(random[2] * scale);
        } else {
            state.words[50] = std::bit_cast<std::uint32_t>(random[0]);
            state.words[52] = std::bit_cast<std::uint32_t>(random[2]);
        }
        state.words[51] = 0u;
    } catch (...) {
        runtime.CleanupBase(state);
        throw;
    }
    return state;
}

void ObjectRandomMovementLifecycle::Cleanup(
    ObjectRandomMovementState& state,
    IObjectRandomMovementRuntime& runtime) {
    state.words[0] = kVtable;
    runtime.CleanupBase(state);
}

std::uint32_t ObjectRandomMovementLifecycle::Delete(
    ObjectRandomMovementState& state, std::uint32_t object_address,
    std::uint32_t flags, IObjectRandomMovementRuntime& runtime) {
    Cleanup(state, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object_address);
    return object_address;
}

}  // namespace pal4
