#include "pal4/showobj_task.h"

namespace pal4 {

ShowobjTaskState& ShowobjTask::Construct(ShowobjTaskState& state,
                                         IShowobjTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

std::string_view ShowobjTask::ClassName() { return "ShowobjTask"; }

ShowobjTaskState& ShowobjTask::Destroy(ShowobjTaskState& state,
                                       IShowobjTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::uint8_t ShowobjTask::Start(ShowobjTaskState& state,
                                 IShowobjTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    runtime.Show(facet);
    return 1u;
}

}  // namespace pal4
