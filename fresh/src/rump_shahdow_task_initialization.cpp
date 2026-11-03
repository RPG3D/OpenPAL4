#include "pal4/rump_shahdow_task.h"

#include "rump_shahdow_task_state.h"

#include <stdexcept>

namespace pal4 {
using namespace rump_shahdow_detail;

std::int32_t RumpShahdowTask::InitializeShadows(
    RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime) {
    const auto owner = state.words[16u];
    const auto facet = runtime.ResolveFacet(owner, ClassName());
    if (facet == 0u)
        throw std::runtime_error("RumpShahdowTask facet unavailable");
    state.words[18u] = facet;
    runtime.PrepareOwner(owner);
    const auto color_facet = runtime.ResolveFacet(owner, "ChangeColorTask");
    if (color_facet == 0u)
        throw std::runtime_error("ChangeColorTask facet unavailable");
    const auto owner_color = runtime.ReadColor(color_facet);
    const auto count = static_cast<std::int32_t>(state.words[23u]);
    if (!runtime.ValidateCount(facet, count))
        throw std::runtime_error("RumpShahdowTask count rejected");

    for (std::int32_t index = 0; index < count; ++index) {
        const auto record_address = runtime.AllocateRecord(0xcu);
        RumpShahdowRecord record{};
        record.age = ReadFloat(state, 20u);
        record.original_alpha = owner_color[3u];
        record.child = runtime.CreateChild(facet, index);
        if (record.child == 0u) {
            runtime.FreeRecord(record_address);
            runtime.ClearRecords(state);
            throw std::runtime_error("RumpShahdowTask child creation failed");
        }
        runtime.WriteRecord(record_address, record);
        const auto child_color = runtime.ResolveFacet(
            record.child, "ChangeColorTask");
        auto initial_color = owner_color;
        initial_color[3u] = 0u;
        runtime.WriteColor(child_color, initial_color);
        runtime.AppendRecord(state, record_address);
    }
    return count;
}

}  // namespace pal4
