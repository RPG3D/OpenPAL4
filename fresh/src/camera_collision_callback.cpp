#include "pal4/camera_collision_callback.h"

namespace pal4 {

bool CameraCollisionCallback::Probe(
    const float* query, float& limit,
    std::array<std::uint32_t, 3>& point) {
    const auto manager = lifecycle_.Register(registration_);
    return scanner_.FindNearest(runtime_.ControllerAt(manager),
                                query, limit, point);
}

}  // namespace pal4
