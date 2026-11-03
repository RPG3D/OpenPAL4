#include "pal4/game_world_pixel_ray.h"

namespace pal4 {

std::uint32_t GameWorldPixelRay::Intersect(
    const GameWorldObjectState& state, const std::uint32_t camera,
    const std::uint32_t pixel_x, const std::uint32_t pixel_y,
    GameWorldVectorBits* intersection, IGameWorldPixelRayRuntime& runtime) {
    if (intersection == nullptr)
        return 0u;

    intersection->fill(kDefaultCoordinateBits);
    GameWorldRayBits ray{};
    runtime.CalculatePixelRay(camera, ray, {pixel_x, pixel_y});
    GameWorldCollisionBits collision{};
    const auto result = GameWorldCollision::Process(
        &ray, state.words[4], collision, runtime);
    if (result != 0u)
        *intersection = {collision[3], collision[4], collision[5]};
    return result;
}

}  // namespace pal4
