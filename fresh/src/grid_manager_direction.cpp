#include "pal4/grid_manager.h"
#include "pal4/grid_vector_math.h"

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
    const volatile float ratio=current_speed/limit;
    const float power=static_cast<float>(runtime.Power(static_cast<double>(ratio),20.0));
    const volatile float scaled_power=-2.f*power;
    const float threshold=scaled_power+1.f;
    const auto normal = runtime.ReadNormal(state);
    return GridVectorMath::ConstrainDirection(
        destination, true, velocity, threshold, normal);
}

}  // namespace pal4
