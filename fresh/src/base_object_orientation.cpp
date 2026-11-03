#include "pal4/base_object_orientation.h"

#include <bit>

namespace pal4 {

void BaseObjectOrientation::Update(
    BaseObjectState& state, const BaseObjectVector3& direction,
    std::uint32_t, std::uint32_t,
    const BaseObjectVector3& next_direction,
    IBaseObjectOrientationRuntime& runtime) {
    if (state.Read(0x14u) == 0u) return;

    BaseObjectVector3 normalized = direction;
    BaseObjectVector3 pose{
        std::bit_cast<float>(state.Read(0xa0u)),
        std::bit_cast<float>(state.Read(0xa4u)),
        std::bit_cast<float>(state.Read(0xa8u))};
    const BaseObjectVector3 copied_direction = next_direction;
    runtime.NormalizeAndClamp(normalized);
    double radians = runtime.AngleFromCosine(
        0.0 * static_cast<double>(normalized.y) +
        0.0 * static_cast<double>(normalized.x) +
        static_cast<double>(normalized.z));
    if (normalized.x < 0.0f) {
        radians = 6.283185307179586 - radians;
    }
    pose.y = static_cast<float>(
        static_cast<double>(57.295780181884766f) * radians);
    runtime.SetPose(state, pose, 0u);
    runtime.SetDirection(state, copied_direction, 0u);
}

}  // namespace pal4
