#include "pal4/object_random_movement_update.h"

#include "pal4/object_random_movement_collision.h"

#include <bit>

namespace pal4 {

BehaviorVector3 ObjectRandomMovementUpdateRuntimeStub::ProcessCollision(
    ObjectRandomMovementState& state, std::uint32_t, float delta_time) {
    ObjectRandomMovementCollisionRuntimeStub runtime;
    BehaviorVector3 output{};
    ObjectRandomMovementCollision::Process(state, output, delta_time, {}, runtime);
    return output;
}
void ObjectRandomMovementUpdateRuntimeStub::UpdateBase(
    ObjectRandomMovementState&, std::uint32_t, float,
    const BehaviorVector3&) {}
std::uint32_t ObjectRandomMovementUpdateRuntimeStub::MemberVtable(
    std::uint32_t) { return 0u; }
std::uint32_t ObjectRandomMovementUpdateRuntimeStub::PositionToken(
    ObjectRandomMovementState&) { return 0u; }
std::uint32_t ObjectRandomMovementUpdateRuntimeStub::NotifyMember(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t ObjectRandomMovementUpdate::Update(
    ObjectRandomMovementState& state, std::uint32_t input,
    float delta_time, std::uint32_t inactive_return,
    IObjectRandomMovementUpdateRuntime& runtime) {
    const auto duration = std::bit_cast<float>(state.words[47]);
    if (duration <= 0.0f) return inactive_return;

    const auto result = runtime.ProcessCollision(state, input, delta_time);
    runtime.UpdateBase(state, input, delta_time, result);
    const auto member_vtable = runtime.MemberVtable(state.words[42]);
    const auto position_token = runtime.PositionToken(state);
    const auto callback_result = runtime.NotifyMember(
        member_vtable, state.words[42], position_token);
    state.words[46] = (state.words[46] & 0xffff00ffu) | 0x100u;
    return callback_result;
}

}  // namespace pal4
