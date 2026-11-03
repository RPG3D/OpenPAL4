#include "pal4/clump_vector_motion.h"

#include <cmath>

namespace pal4 {

ClumpMotionVector& ClumpVectorMotion::Initialize(
    ClumpVectorMotionState& state, ClumpMotionVector& output,
    const ClumpMotionVector& start, const ClumpMotionVector& end,
    const float launch_value, const float gravity,
    IClumpVectorMotionRuntime& runtime) {
    state.start = start;
    state.end = end;
    state.launch_value = launch_value < 1.0f ? 1.0f : launch_value;
    state.gravity = gravity;

    //5FB86C..8FD runs with the game's PC24 control word. Keep each arithmetic
    //boundary at float precision, including the stored duration before the
    //horizontal velocity calculation. Public sqrt handles the root itself.
    const float reciprocal_gravity = 1.0f / gravity;
    const float initial_time = reciprocal_gravity * state.launch_value;
    const float twice_initial = initial_time + initial_time;
    const float base_time = std::sqrt(twice_initial);
    float secondary_time;
    if (end.y > start.y) {
        const float height_difference = end.y - start.y;
        const float remaining_height = state.launch_value - height_difference;
        const float magnitude = std::fabs(remaining_height);
        const float twice_magnitude = magnitude + magnitude;
        const float root_input = twice_magnitude / gravity;
        secondary_time = std::sqrt(root_input) + initial_time;
    } else if (start.y > end.y) {
        const float twice_gravity = gravity + gravity;
        const float fraction = base_time / twice_gravity;
        const float height_difference = start.y - end.y;
        const float sum = fraction + height_difference;
        const float scaled = sum * reciprocal_gravity;
        const float twice_scaled = scaled + scaled;
        secondary_time = std::sqrt(twice_scaled) + initial_time;
    } else {
        const float height_difference = end.y - start.y;
        const float magnitude = std::fabs(height_difference);
        const float sum = magnitude + state.launch_value;
        const float twice_sum = sum + sum;
        const float root_input = twice_sum / gravity;
        secondary_time = std::sqrt(root_input);
    }
    const float duration = secondary_time + base_time;
    const float double_duration = duration + duration;

    state.velocity = {end.x - start.x, 0.0f, end.z - start.z};
    runtime.Normalize(state.velocity);
    const ClumpMotionVector horizontal_delta{
        end.x - start.x, 0.0f, end.z - start.z};
    const auto speed = [&] {
        return static_cast<float>(runtime.ApproximateMagnitude(horizontal_delta)) /
               double_duration;
    };
    state.velocity.x = speed() * state.velocity.x;
    state.velocity.y = speed() * state.velocity.y;
    state.velocity.z = speed() * state.velocity.z;
    state.velocity.y = duration * state.gravity;
    output = state.velocity;
    return output;
}

ClumpMotionVector& ClumpVectorMotion::ScaleAndSetStatus(
    ClumpVectorMotionState& state, ClumpMotionVector& output,
    const float scale) {
    output = {scale * state.velocity.x, scale * state.velocity.y,
              scale * state.velocity.z};
    state.velocity.y -= scale * state.gravity;
    state.status = state.velocity.y <= 0.0f ? 2u : 1u;
    return output;
}

}  // namespace pal4
