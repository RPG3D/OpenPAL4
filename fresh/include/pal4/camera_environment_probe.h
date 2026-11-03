#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>

namespace pal4 {

using CameraSegment = std::array<float, 6>;
using CameraPoint = std::array<float, 3>;

struct CameraCollision {
    float fraction{};
    CameraPoint point{};
};

class ICameraEnvironmentRuntime {
public:
    virtual ~ICameraEnvironmentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EnvironmentFrame(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual bool ProbeAuxiliary(
        std::uint32_t callback, const CameraSegment& segment,
        float& fraction, CameraPoint& point) = 0;
    [[nodiscard]] virtual bool Intersect(
        std::uint32_t frame, const CameraSegment& segment,
        CameraCollision& collision) = 0;
    [[nodiscard]] virtual float Magnitude(const CameraPoint& vector) = 0;
};

class CameraEnvironmentProbe final {
public:
    explicit CameraEnvironmentProbe(ICameraEnvironmentRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ea710: select the nearest of four environment probes.
    [[nodiscard]] bool Update(CameraDataState& state, float* distance,
                              CameraPoint* relative_direction);

private:
    [[nodiscard]] bool UpdateUnobserved(CameraDataState& state, float* distance,
                                        CameraPoint* relative_direction);
    ICameraEnvironmentRuntime& runtime_;
};

}  // namespace pal4
