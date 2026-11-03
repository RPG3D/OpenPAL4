#include "pal4/rump_shahdow_task.h"

#include "rump_shahdow_task_state.h"

#include <stdexcept>

namespace pal4 {
namespace {

using namespace rump_shahdow_detail;

bool TransformUnchanged(const RumpShahdowTaskState& state,
                        const RumpShahdowTransform& transform,
                        const RumpShahdowColor& color) {
    for (std::size_t i = 0; i < 3u; ++i) {
        if (ReadByte(state, 180u + i) != color[i])
            return false;
    }
    for (const auto i : {4u, 5u, 6u, 8u, 9u, 10u,
                         0u, 1u, 2u, 12u, 13u, 14u}) {
        const auto previous = ReadFloat(state, 29u + i);
        if (!(transform[i] + 0.01f > previous &&
              transform[i] - 0.01f < previous))
            return false;
    }
    return true;
}

void SaveTransform(RumpShahdowTaskState& state,
                   const RumpShahdowTransform& transform,
                   const RumpShahdowColor& color) {
    for (std::size_t i = 0; i < 16u; ++i)
        WriteFloat(state, 29u + i, transform[i]);
    for (std::size_t i = 0; i < 3u; ++i)
        WriteByte(state, 180u + i, color[i]);
}

void FadeShadows(RumpShahdowTaskState& state, const float elapsed,
                 IRumpShahdowTaskRuntime& runtime) {
    const auto count = runtime.RecordCount(state);
    const auto lifetime = ReadFloat(state, 20u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto address = runtime.RecordAt(state, index);
        auto record = runtime.ReadRecord(address);
        record.age += elapsed;
        if (record.age >= lifetime)
            record.age = lifetime;
        runtime.WriteRecord(address, record);
        const auto color_facet = runtime.ResolveFacet(
            record.child, "ChangeColorTask");
        auto color = runtime.ReadColor(color_facet);
        const auto scaled_alpha =
            (1.0 - static_cast<double>(index) / static_cast<double>(count)) *
            (1.0 - static_cast<double>(record.age) /
                       static_cast<double>(lifetime)) *
            static_cast<double>(record.original_alpha);
        color[3u] = static_cast<std::uint8_t>(
            runtime.FloatToInt(scaled_alpha));
        runtime.WriteColor(color_facet, color);
    }
}

}  // namespace

std::uint8_t RumpShahdowTask::Update(
    RumpShahdowTaskState& state, const float elapsed,
    IRumpShahdowTaskRuntime& runtime) {
    using namespace rump_shahdow_detail;
    WriteFloat(state, 22u, elapsed + ReadFloat(state, 22u));
    if (ReadFloat(state, 22u) > ReadFloat(state, 21u)) {
        state.words[22u] = 0u;
        const auto transform = runtime.ReadOwnerTransform(state.words[16u]);
        const auto color_facet = runtime.ResolveFacet(
            state.words[16u], "ChangeColorTask");
        const auto color = runtime.ReadColor(color_facet);
        bool spawn = true;
        if (ReadFlag(state, 112u)) {
            if (ReadFlag(state, 113u)) {
                WriteFlag(state, 113u, false);
            } else {
                spawn = !TransformUnchanged(state, transform, color);
            }
            if (spawn)
                SaveTransform(state, transform, color);
        }
        if (spawn) {
            if (runtime.RecordCount(state) == 0u)
                throw std::runtime_error("RumpShahdowTask has no reusable child");
            const auto address = runtime.MoveLastRecordToFront(state);
            auto record = runtime.ReadRecord(address);
            record.age = 0.0f;
            record.original_alpha = color[3u];
            runtime.WriteRecord(address, record);
            const auto frame = runtime.ChildFrame(record.child);
            runtime.FrameSetIdentity(frame);
            runtime.FrameApplyTransform(frame, transform);
            runtime.FrameUpdateObjects(frame);
            const auto child_color = runtime.ResolveFacet(
                record.child, "ChangeColorTask");
            runtime.WriteColor(child_color, color);
        }
    }
    FadeShadows(state, elapsed, runtime);
    return 1u;
}

}  // namespace pal4
