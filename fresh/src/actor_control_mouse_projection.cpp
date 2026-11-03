#include "pal4/actor_control_mouse_projection.h"

#include <bit>
#include <limits>

namespace pal4 {

ActorControlCursorPoint ActorControlMouseProjectionRuntimeStub::CursorPosition() {
    return {};
}
void ActorControlMouseProjectionRuntimeStub::ScreenToClient(
    ActorControlCursorPoint&) {}
std::uint32_t ActorControlMouseProjectionRuntimeStub::CameraSystem() {
    return 0u;
}
std::uint32_t ActorControlMouseProjectionRuntimeStub::ActiveCameraInternalId(
    std::uint32_t) { return 0u; }
void ActorControlMouseProjectionRuntimeStub::CalculateNormalizedPixelRay(
    std::uint32_t, float, float, ActorControlRay&) {}
std::uint32_t ActorControlMouseProjectionRuntimeStub::ReadActorWord(
    std::uint32_t) { return 0u; }
double ActorControlMouseProjectionRuntimeStub::CalculateIntersection(
    const ActorControlRay&, const ActorControlVec3&,
    const ActorControlVec3&) {
    return static_cast<double>(std::numeric_limits<float>::max());
}

bool ActorControlMouseProjection::Project(
    std::uint32_t actor, ActorControlVec3& output,
    IActorControlMouseProjectionRuntime& runtime) {
    auto cursor = runtime.CursorPosition();
    runtime.ScreenToClient(cursor);
    ActorControlRay ray{};
    const auto camera = runtime.ActiveCameraInternalId(runtime.CameraSystem());
    runtime.CalculateNormalizedPixelRay(
        camera, static_cast<float>(cursor.x),
        static_cast<float>(cursor.y), ray);

    const ActorControlVec3 plane_normal{0.0f, 1.0f, 0.0f};
    const ActorControlVec3 actor_position{
        std::bit_cast<float>(runtime.ReadActorWord(actor + 0xacu)),
        std::bit_cast<float>(runtime.ReadActorWord(actor + 0xb0u)),
        std::bit_cast<float>(runtime.ReadActorWord(actor + 0xb4u))};
    const double distance = runtime.CalculateIntersection(
        ray, plane_normal, actor_position);
    if (!(distance < static_cast<double>(std::numeric_limits<float>::max())))
        return false;

    output.x = static_cast<float>(
        static_cast<double>(ray.direction.x) * distance + ray.origin.x);
    output.y = static_cast<float>(
        static_cast<double>(ray.direction.y) * distance + ray.origin.y);
    output.z = static_cast<float>(
        static_cast<double>(ray.direction.z) * distance + ray.origin.z);
    return true;
}

}  // namespace pal4
