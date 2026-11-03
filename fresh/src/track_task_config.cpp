#include "pal4/track_task.h"

#include "track_task_state.h"

#include <array>
#include <cstdint>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 3u> kPositionKeys{
    "Attr_Task_Track_Position_x", "Attr_Task_Track_Position_y",
    "Attr_Task_Track_Position_z"};

}  // namespace

bool TrackTask::LoadConfig(TrackTaskState& state, std::uint32_t config,
                            TrackPointArray& points,
                            ITrackTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;

    track_task_detail::WriteFlag(state, 61u,
        runtime.ReadInt(config, "Attr_Inherit_Pre_Task", 0) > 0);
    TrackPointArray raw_points;
    for (const auto child : runtime.Children(
             config, "Attr_Task_Track_Position")) {
        TrackPoint point{};
        for (std::size_t i = 0; i < point.size(); ++i)
            point[i] = runtime.ReadFloat(child, kPositionKeys[i], 0.0f);
        static_cast<void>(raw_points.InsertOne(raw_points.Size(), point));
    }
    if (raw_points.Size() < 4u)
        throw std::runtime_error(
            "create track task failed,because the track points is less than 4...");

    track_task_detail::WriteFloat(state, 33u,
        runtime.ReadFloat(config, "Attr_Task_Track_Time", 5.0f));
    track_task_detail::WriteFloat(state, 34u,
        runtime.ReadFloat(config, "Attr_Task_Track_Min_Time", 0.0f));
    track_task_detail::WriteFlag(state, 74u,
        runtime.ReadInt(config, "Attr_Task_Track_Result", 0) > 0);
    const auto sample = runtime.ReadInt(
        config, "Attr_Task_Track_Sample", 1) > 0;
    const auto coefficient = runtime.ReadInt(
        config, "Attr_Task_Track_Sample_Coeff", 4);
    const auto type = runtime.ReadInt(
        config, "Attr_Task_Track_Type", 2);

    if (sample) {
        const auto spline = runtime.CreateSpline(raw_points, type);
        try {
            const auto count = static_cast<std::uint32_t>(raw_points.Size()) *
                (static_cast<std::uint32_t>(coefficient) + 1u);
            for (std::uint32_t i = 0; i < count; ++i) {
                const auto time = static_cast<float>(i) /
                    static_cast<float>(count - 1u);
                const auto point = runtime.SampleSpline(spline, 10, time);
                static_cast<void>(points.InsertOne(points.Size(), point));
            }
        } catch (...) {
            runtime.DestroySpline(spline);
            throw;
        }
        runtime.DestroySpline(spline);
    } else {
        for (const auto& point : raw_points.Points())
            static_cast<void>(points.InsertOne(points.Size(), point));
        if (type == 2)
            static_cast<void>(points.InsertOne(points.Size(),
                                               raw_points.At(0u)));
    }
    runtime.SynchronizePointStorage(state, points);
    track_task_detail::WriteFlag(state, 72u,
        runtime.ReadTreeInt(config, "Attr_Task_Track_Loop", 0) > 0);
    track_task_detail::WriteFlag(state, 73u,
        runtime.ReadInt(config, "Attr_Task_Track_Autoface", 0) > 0);
    return true;
}

}  // namespace pal4
