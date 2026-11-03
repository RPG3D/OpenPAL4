#pragma once

#include "pal4/camera_data.h"

#include <cstdint>

namespace pal4 {

class CameraFrustumGeometry final {
public:
    // 0x5ea420: create four front/back vertex pairs from camera basis vectors.
    [[nodiscard]] static std::uint32_t UpdateVertices(
        CameraDataState& state, float depth);
};

}  // namespace pal4
