#include "pal4/hideobj_task.h"

namespace pal4 {

HideobjTaskState& HideobjTask::Construct(HideobjTaskState& state,
                                        IHideobjTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    state.words[18u] = 0u;
    return state;
}

std::string_view HideobjTask::ClassName() {
    return "HideobjTask";
}

HideobjTaskState& HideobjTask::Destroy(HideobjTaskState& state,
                                      IHideobjTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::uint8_t HideobjTask::Start(HideobjTaskState& state,
                                IHideobjFacetRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    runtime.Hide(facet);
    return 1u;
}

}  // namespace pal4
