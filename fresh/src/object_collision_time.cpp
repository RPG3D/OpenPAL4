#include "pal4/object_collision_time.h"

#include <bit>
#include <cmath>

namespace pal4 {

BehaviorVector3 ObjectCollisionTimeRuntimeStub::RelativeVector(
    ObjectMovementState&, std::uint32_t) { return {}; }
float ObjectCollisionTimeRuntimeStub::ObjectRadius(ObjectMovementState&) {
    return 0.0f;
}
float ObjectCollisionTimeRuntimeStub::CandidateRadius(std::uint32_t) {
    return 0.0f;
}

void ObjectCollisionTime::Calculate(
    ObjectMovementState& state, std::uint32_t candidate_address,
    ObjectCollisionCandidate& result, IObjectCollisionTimeRuntime& runtime) {
    result.words[0] = 0u;
    result.words[8] = candidate_address;
    const auto vector = runtime.RelativeVector(state, candidate_address + 8u);
    const float twice_negative_z = vector[2] * -2.0f;
    const float object_radius = runtime.ObjectRadius(state);
    const float candidate_radius = runtime.CandidateRadius(candidate_address);
    const double radius = static_cast<double>(object_radius) + candidate_radius;
    const double square = static_cast<double>(vector[0]) * vector[0] +
                          static_cast<double>(vector[1]) * vector[1] +
                          static_cast<double>(vector[2]) * vector[2];
    const double discriminant =
        static_cast<double>(twice_negative_z) * twice_negative_z -
        (square - radius * radius) * 4.0;
    if (discriminant < 0.0) return;

    const double root = std::sqrt(discriminant);
    const float plus = static_cast<float>((root - twice_negative_z) * 0.5);
    const double minus = (-static_cast<double>(twice_negative_z) - root) * 0.5;
    if (plus < 0.0f && minus < 0.0) return;

    result.words[0] = 1u;
    double chosen = minus;
    if (plus > 0.0f) {
        if (!(minus > 0.0) || plus < minus)
            chosen = plus;
    }
    result.words[1] = std::bit_cast<std::uint32_t>(
        static_cast<float>(chosen));
}

}  // namespace pal4
