#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>

namespace pal4 {

using CameraBoundsVertices = std::array<std::array<float, 3>, 8>;
using CameraBoundsMatrix = std::array<std::uint32_t, 16>;

class ICameraBoundsRuntime {
public:
    virtual ~ICameraBoundsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CalculateBoundingBox(
        CameraDataState& state, std::uint32_t bounds_offset,
        const CameraBoundsVertices& vertices,
        const CameraBoundsMatrix& transform,
        std::uint32_t camera) = 0;
};

class CameraBounds final {
public:
    explicit CameraBounds(ICameraBoundsRuntime& runtime) : runtime_(runtime) {}

    // 0x5ea910: prepare frustum points and an identity transform for bounds.
    [[nodiscard]] std::uint32_t Prepare(CameraDataState& state);

private:
    ICameraBoundsRuntime& runtime_;
};

}  // namespace pal4
