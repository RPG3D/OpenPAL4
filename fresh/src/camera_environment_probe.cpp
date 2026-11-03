#include "pal4/camera_environment_probe.h"
#include "pal4/comparison_camera_collision_trace.h"

#include <bit>
#include <cfloat>
#include <cstddef>

namespace pal4 {
namespace {

float ReadFloat(const CameraDataState& state, std::size_t offset) {
    return std::bit_cast<float>(state.At(offset));
}

void WriteFloat(CameraDataState& state, std::size_t offset, float value) {
    state.At(offset) = std::bit_cast<std::uint32_t>(value);
}

CameraPoint ReadPoint(const CameraDataState& state, std::size_t offset) {
    return {ReadFloat(state, offset), ReadFloat(state, offset + 4u),
            ReadFloat(state, offset + 8u)};
}

void WritePoint(CameraDataState& state, std::size_t offset,
                const CameraPoint& point) {
    for (std::size_t axis = 0; axis < point.size(); ++axis)
        WriteFloat(state, offset + axis * 4u, point[axis]);
}

}  // namespace

bool CameraEnvironmentProbe::Update(CameraDataState& state, float* distance,
                                    CameraPoint* relative_direction) {
    auto& trace=GetComparisonCameraCollisionTrace();
    if(!trace.Enabled())return UpdateUnobserved(state,distance,relative_direction);
    ComparisonCameraCollisionScope scope(trace,&state);
    const auto result=UpdateUnobserved(state,distance,relative_direction);
    scope.Complete(result);trace.Publish("native");return result;
}

bool CameraEnvironmentProbe::UpdateUnobserved(CameraDataState& state, float* distance,
                                              CameraPoint* relative_direction) {
    const auto camera = state.At(0);
    if (runtime_.EnvironmentFrame(camera) == 0u) return false;

    state.At(0x2d4u) = 0u;
    WriteFloat(state, 0x2d8u, FLT_MAX);
    state.At(0x2e8u) = 0xffffffffu;

    for (std::uint32_t index = 0u; index < 4u; ++index) {
        if (state.At(0x1acu) != 0u)
            state.At(0x220u + index * 4u) = 0xff0000ffu;

        CameraSegment segment{};
        for (std::size_t axis = 0; axis < segment.size(); ++axis)
            segment[axis] = ReadFloat(
                state, 0x1c0u + index * 24u + axis * 4u);

        const auto callback = state.At(0x2bcu);
        if (callback != 0u) {
            auto fraction = ReadFloat(state, 0x2d8u);
            auto point = ReadPoint(state, 0x2dcu);
            const auto found = runtime_.ProbeAuxiliary(
                callback, segment, fraction, point);
            WriteFloat(state, 0x2d8u, fraction);
            WritePoint(state, 0x2dcu, point);
            if (found) {
                state.At(0x2e8u) = index;
                state.At(0x2d4u) = 1u;
            }
        }

        CameraCollision collision{};
        state.At(0x2c0u) = 0u;
        WriteFloat(state, 0x2c4u, FLT_MAX);
        const auto frame = runtime_.EnvironmentFrame(camera);
        if (runtime_.Intersect(frame, segment, collision)) {
            state.At(0x2c0u) = 1u;
            WriteFloat(state, 0x2c4u, collision.fraction);
            WritePoint(state, 0x2c8u, collision.point);
            if (collision.fraction < ReadFloat(state, 0x2d8u)) {
                WriteFloat(state, 0x2d8u, collision.fraction);
                WritePoint(state, 0x2dcu, collision.point);
                state.At(0x2e8u) = index;
                state.At(0x2d4u) = 1u;
            }
        }
    }

    if (state.At(0x2d4u) == 0u) return false;

    const auto point = ReadPoint(state, 0x2dcu);
    WritePoint(state, 0x2acu, point);
    CameraPoint relative{};
    for (std::size_t axis = 0; axis < relative.size(); ++axis)
        relative[axis] = static_cast<float>(
            static_cast<double>(ReadFloat(state, 0x74u + axis * 4u)) -
            point[axis]);
    WritePoint(state, 0x2dcu, relative);

    auto magnitude = runtime_.Magnitude(relative);
    if (magnitude < ReadFloat(state, 0x1b8u))
        magnitude = ReadFloat(state, 0x1b8u);
    WriteFloat(state, 0x2d8u, magnitude);
    WriteFloat(state, 0x2b8u, magnitude);
    if (distance != nullptr) *distance = magnitude;
    if (relative_direction != nullptr) *relative_direction = relative;

    if (state.At(0x1acu) != 0u)
        state.At(0x220u + state.At(0x2e8u) * 4u) = 0xff00ff00u;
    return true;
}

}  // namespace pal4
