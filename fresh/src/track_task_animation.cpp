#include "pal4/track_task.h"

#include "track_task_state.h"

#include <cmath>
#include <cstddef>

namespace pal4 {
using namespace track_task_detail;
namespace {

constexpr float kNativeTolerance = 0.0099999998f;

bool NearZero(const TrackPoint& point) {
    for (const auto component : point) {
        if (!(component >= -kNativeTolerance &&
              component < kNativeTolerance))
            return false;
    }
    return true;
}

bool MatchesDirection(const TrackPoint& normalized,
                      const TrackPoint& direction) {
    for (std::size_t i = 0; i < direction.size(); ++i) {
        if (!(normalized[i] - kNativeTolerance < direction[i] &&
              direction[i] <= normalized[i] + kNativeTolerance))
            return false;
    }
    return true;
}

void AdvanceSegment(TrackTaskState& state, const TrackPointArray& points,
                    ITrackTaskRuntime& runtime) {
    const auto next = static_cast<std::size_t>(state.words[22u]) + 1u;
    state.words[22u] = static_cast<std::uint32_t>(next);
    if (next >= points.Size()) {
        if (!ReadFlag(state, 72u))
            return;
        state.words[22u] = 1u;
        WritePoint(state, 30u, points.At(0u));
        state.words[21u] = 0u;
    } else {
        WritePoint(state, 30u, points.At(next - 1u));
    }
    const auto index = static_cast<std::size_t>(state.words[22u]);
    WritePoint(state, 27u, runtime.Normalize(Difference(
        points.At(index), points.At(index - 1u))));
}

}  // namespace

std::uint8_t TrackTask::Start(TrackTaskState& state, TrackPointArray& points,
                               ITrackTaskRuntime& runtime) {
    runtime.TransformPoints(state.words[17u], points);
    double length = 0.0;
    for (std::size_t i = 1u; i < points.Size(); ++i) {
        const auto difference = Difference(points.At(i), points.At(i - 1u));
        const auto x = static_cast<double>(difference[0]);
        const auto y = static_cast<double>(difference[1]);
        const auto z = static_cast<double>(difference[2]);
        length += std::sqrt(x * x + y * y + z * z);
    }
    const auto speed = length / static_cast<double>(ReadFloat(state, 33u));
    const auto min_time = static_cast<double>(ReadFloat(state, 34u));
    double acceleration = 0.0;
    if (min_time > 0.0) {
        if (ReadFlag(state, 74u)) {
            const auto final_speed = length / min_time;
            acceleration = (final_speed - speed) /
                (length / ((final_speed + speed) * 0.5));
        } else {
            acceleration = (2.0 * (length - speed * min_time)) /
                           (min_time * min_time);
        }
    }
    WriteFloat(state, 19u, static_cast<float>(speed));
    WriteFloat(state, 20u, static_cast<float>(acceleration));
    WritePoint(state, 30u, points.At(0u));
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[35u] = facet;
    runtime.SetPosition(facet, ReadPoint(state, 30u));
    state.words[22u] = 1u;
    WritePoint(state, 27u, runtime.Normalize(Difference(
        points.At(1u), points.At(0u))));
    if (ReadFlag(state, 73u))
        runtime.FacePoint(facet, points, 1u);
    state.words[21u] = 0u;
    return 1u;
}

std::uint8_t TrackTask::Update(TrackTaskState& state, float elapsed,
                                const TrackPointArray& points,
                                ITrackTaskRuntime& runtime) {
    const auto index = static_cast<std::size_t>(state.words[22u]);
    if (index >= points.Size())
        return 1u;

    const auto old_time = static_cast<double>(ReadFloat(state, 21u));
    const auto acceleration = static_cast<double>(ReadFloat(state, 20u));
    const auto speed = static_cast<double>(ReadFloat(state, 19u));
    const auto new_time = old_time + static_cast<double>(elapsed);
    WriteFloat(state, 21u, static_cast<float>(new_time));
    const auto distance = (speed + acceleration * new_time * 0.5) * new_time -
                          (speed + acceleration * old_time * 0.5) * old_time;
    auto position = ReadPoint(state, 30u);
    const auto direction = ReadPoint(state, 27u);
    for (std::size_t i = 0; i < position.size(); ++i)
        position[i] = static_cast<float>(
            static_cast<double>(position[i]) +
            static_cast<double>(direction[i]) * distance);
    WritePoint(state, 30u, position);

    const auto remaining = Difference(points.At(index), position);
    if (NearZero(remaining) ||
        !MatchesDirection(runtime.Normalize(remaining), direction))
        AdvanceSegment(state, points, runtime);

    runtime.SetPosition(state.words[35u], ReadPoint(state, 30u));
    if (ReadFlag(state, 73u))
        runtime.FacePoint(state.words[35u], points,
                          static_cast<std::size_t>(state.words[22u]));
    return 1u;
}

}  // namespace pal4
