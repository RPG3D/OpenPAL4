#include "pal4/grid_manager.h"
#include "pal4/grid_vector_math.h"

#include <cmath>

namespace pal4 {

GridVector3& GridManager::ConstrainVelocity(
    const GridManagerState& state, GridVector3& destination,
    const GridVector3& velocity,
    IGridManagerDirectionRuntime& runtime) {
    const float limit = runtime.ReadParameter(state, 120u) * 0.2f;
    const float speed = runtime.ReadParameter(state, 100u);
    if (!(speed <= limit) || velocity == runtime.ReadZeroVector() ||
        limit == 0.0f) {
        destination = velocity;
        return destination;
    }
    const float current_speed = runtime.ReadParameter(state, 100u);
    const float power = static_cast<float>(std::pow(
        static_cast<double>(current_speed) / limit, 20.0));
    const float threshold = 1.0f - 2.0f * power;
    const auto normal = runtime.ReadNormal(state);
    return GridVectorMath::ConstrainDirection(
        destination, true, velocity, threshold, normal);
}

}  // namespace pal4
