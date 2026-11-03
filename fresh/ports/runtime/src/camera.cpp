#include "pal4/portable/scene.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
using Vector = std::array<float, 3>;
Vector Cross(Vector a, Vector b) {
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
float Dot(Vector a, Vector b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vector Normalize(Vector value) {
    const auto length = std::sqrt(Dot(value, value));
    for (auto& component : value) component /= length;
    return value;
}
}
void OrbitCamera::Fit(const Scene& scene) {
    float diagonal = 0;
    for (int axis = 0; axis < 3; ++axis) {
        target[axis] = (scene.minimum[axis] + scene.maximum[axis]) * .5f;
        const auto extent = scene.maximum[axis] - scene.minimum[axis];
        diagonal += extent * extent;
    }
    scale = std::max(std::sqrt(diagonal), 1.f);
    distance = scale * .65f;
    yaw = .7f; pitch = .45f;
}
void OrbitCamera::Orbit(float horizontal, float vertical) {
    if (!std::isfinite(horizontal) || !std::isfinite(vertical)) return;
    yaw = std::remainder(yaw + horizontal, 6.283185307f);
    pitch = std::clamp(pitch + vertical, -1.45f, 1.45f);
}
void OrbitCamera::Zoom(float delta) {
    if (std::isfinite(delta))
        distance = std::clamp(distance * std::exp(std::clamp(delta, -3.f, 3.f)), scale*.01f, scale*5.f);
}
std::array<float,16> OrbitCamera::Matrix(float aspect) const {
    if (!std::isfinite(aspect) || aspect <= 0) throw std::runtime_error("invalid viewport aspect");
    const Vector offset{distance*std::cos(pitch)*std::sin(yaw), distance*std::sin(pitch),
                        distance*std::cos(pitch)*std::cos(yaw)};
    const Vector eye{target[0]+offset[0], target[1]+offset[1], target[2]+offset[2]};
    const auto forward = Normalize(Vector{-offset[0], -offset[1], -offset[2]});
    const auto right = Normalize(Cross(forward, {0,1,0}));
    const auto up = Cross(right, forward);
    const std::array<float,16> view{
        right[0],up[0],-forward[0],0, right[1],up[1],-forward[1],0,
        right[2],up[2],-forward[2],0, -Dot(right,eye),-Dot(up,eye),Dot(forward,eye),1};
    const float near_plane = std::max(.1f, distance*.001f);
    const float far_plane = std::max(distance + scale*3.f, near_plane + 1.f);
    constexpr float f = 1.732050808f;
    const std::array<float,16> projection{f/aspect,0,0,0, 0,f,0,0, 0,0,
        (far_plane+near_plane)/(near_plane-far_plane),-1, 0,0,
        2*far_plane*near_plane/(near_plane-far_plane),0};
    std::array<float,16> result{};
    for (int col=0; col<4; ++col)
        for (int row=0; row<4; ++row)
            for (int k=0; k<4; ++k) result[col*4+row] += projection[k*4+row]*view[col*4+k];
    return result;
}
}  // namespace pal4::portable
