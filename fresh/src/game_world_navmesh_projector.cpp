#include "pal4/game_world_navmesh_projector.h"

namespace pal4 {

std::uint32_t GameWorldNavmeshProjector::Project(
    const std::uint32_t camera, const std::int32_t x, const std::int32_t y,
    GameWorldPointWords* const output, IGameWorldNavmeshRuntime& runtime) {
    if (output == nullptr) return 0u;
    output->fill(kMissingCoordinate);
    GameWorldPixelRay ray{};
    runtime.CalculatePixelRay(camera, x, y, ray);
    GameWorldCollisionSample sample{};
    const auto result = runtime.Collide(ray, runtime.NavClump(), sample);
    if (result != 0u) {
        (*output)[0] = sample[3];
        (*output)[1] = sample[4];
        (*output)[2] = sample[5];
    }
    return result;
}

}  // namespace pal4
