#include "pal4/actor_manager_screen_projection.h"

#include <limits>

namespace pal4 {

ActorScreenPoint ActorManagerScreenProjection::Project(
    std::uint32_t actor, std::uint32_t camera,
    IActorManagerScreenProjectionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    if (child == 0u) {
        const auto maximum = std::numeric_limits<float>::max();
        return {maximum, maximum};
    }

    const ActorWorldPoint point{
        runtime.ReadFloat(child + 172u),
        runtime.ReadFloat(child + 176u) + 80.0f,
        runtime.ReadFloat(child + 180u)};
    return runtime.ConvertUIFrame(runtime.ProjectCamera(camera, point));
}

}  // namespace pal4
