#include "pal4/camera_obstacle_check.h"

#include <bit>

namespace pal4 {

bool CameraObstacleCheck::RetrieveDistance(CameraDataState& state,
                                           float& distance) {
    if (!runtime_.Intersects(state, 0x230u, 0x2acu, 0u)) return false;
    distance = std::bit_cast<float>(state.At(0x2b8u));
    return true;
}

}  // namespace pal4
