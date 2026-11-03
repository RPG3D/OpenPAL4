#include "pal4/base_object_motion.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(const BaseObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.Read(offset));
}

}  // namespace

std::uint32_t BaseObjectMotion::UpdatePosition(
    BaseObjectState& state, std::uint32_t address,
    float elapsed, std::uint32_t argument,
    IBaseObjectMotionRuntime& runtime) {
    const auto active = state.Read(0x1f0u);
    if (active == 0u) return active;

    runtime.UpdateAndRenderClump(elapsed);
    runtime.UpdateInterpolator(state, 0x170u, elapsed);
    runtime.UpdateCharacter(state, 0x564u, argument, elapsed, 1.0f);
    return ApplyTransform(state,address,runtime);
}
std::uint32_t BaseObjectMotion::ApplyTransform(BaseObjectState& state,
    std::uint32_t address,IBaseObjectMotionRuntime& runtime) {
    const auto hold = state.Read(0x5f4u);
    if (hold != 0u) return hold;

    const BaseObjectVector3 direction{
        ReadFloat(state, 0x570u), ReadFloat(state, 0x574u),
        ReadFloat(state, 0x578u)};
    BaseObjectVector3 pose{
        ReadFloat(state, 0xa0u), ReadFloat(state, 0xa4u),
        ReadFloat(state, 0xa8u)};
    const BaseObjectVector3 forward{
        ReadFloat(state, 0x57cu), ReadFloat(state, 0x580u),
        ReadFloat(state, 0x584u)};
    if (forward.x == 0.0f && forward.z == 0.0f) {
        // The second x87 status store overwrites only AX of the vector pointer.
        return ((address + 0x57cu) & 0xffff0000u) | 0x4000u;
    }

    BaseObjectVector3 flat = forward;
    if (state.Read(0x604u) != 0u) {
        flat.x = -flat.x;
        flat.y = -flat.y;
        flat.z = -flat.z;
    }
    flat.y = 0.0f;
    runtime.NormalizeAndClamp(flat);
    double radians = runtime.AngleFromCosine(flat.z);
    if (flat.x < 0.0f) {
        // IDA442B14/442B16: FCHS then FADD(double2pi), under PC24.
        // Round the complement before multiplying degrees at442B28. Without
        // this boundary, captured M11 motion differs by one stored yaw ULP.
        const volatile float mirrored = static_cast<float>(6.283185307179586 - radians);
        radians = mirrored;
    }
    pose.y = static_cast<float>(radians * 57.295780181884766);
    runtime.SetPose(state, pose, 0u);
    return runtime.SetDirection(state, direction);
}

}  // namespace pal4
