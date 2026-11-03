#pragma once

#include "pal4/camera_data.h"

#include <cstdint>

namespace pal4 {

class ICameraObstacleRuntime {
public:
    virtual ~ICameraObstacleRuntime() = default;
    [[nodiscard]] virtual bool Intersects(
        CameraDataState& state, std::uint32_t bounds_offset,
        std::uint32_t point_offset, std::uint32_t flags) = 0;
};

class CameraObstacleCheck final {
public:
    explicit CameraObstacleCheck(ICameraObstacleRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eaa00: return cached camera distance only when bounds intersect.
    [[nodiscard]] bool RetrieveDistance(CameraDataState& state,
                                        float& distance);

private:
    ICameraObstacleRuntime& runtime_;
};

}  // namespace pal4
