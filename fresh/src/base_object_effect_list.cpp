#include "pal4/base_object_effect_list.h"

namespace pal4 {
namespace {

std::uint32_t Update(BaseObjectState& state, std::uint32_t component,
                     bool resolved, IBaseObjectEffectListRuntime& runtime) {
    if (resolved) {
        const auto id = runtime.QueryComponent(state, component);
        runtime.RemoveById(state, 0x170u, id);
    } else {
        runtime.AddOrReplaceComponent(state, 0x170u, component);
    }
    return 1u;
}

}  // namespace

std::uint32_t BaseObjectEffectList::UpdateFirst(
    BaseObjectState& state, IBaseObjectEffectListRuntime& runtime) {
    return Update(state, 0x210u, false, runtime);
}

std::uint32_t BaseObjectEffectList::UpdateFirstResolved(
    BaseObjectState& state, IBaseObjectEffectListRuntime& runtime) {
    return Update(state, 0x210u, true, runtime);
}

std::uint32_t BaseObjectEffectList::UpdateSecond(
    BaseObjectState& state, IBaseObjectEffectListRuntime& runtime) {
    return Update(state, 0x208u, false, runtime);
}

std::uint32_t BaseObjectEffectList::UpdateSecondResolved(
    BaseObjectState& state, IBaseObjectEffectListRuntime& runtime) {
    return Update(state, 0x208u, true, runtime);
}

}  // namespace pal4
