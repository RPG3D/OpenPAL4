#include "pal4/effect_object_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(const EffectObjectState& state,
                       const std::uint32_t id,
                       const std::uint32_t opaque,
                       const std::uint32_t slot,
                       IEffectObjectQueueRuntime& runtime) {
    EffectSystemHandleWords handle{id, state.words[4u]};
    try {
        runtime.Dispatch(handle.system, slot, handle, opaque);
        return runtime.Reset(handle);
    } catch (...) {
        (void)runtime.Reset(handle);
        throw;
    }
}

}  // namespace

std::uint32_t EffectObjectQueue::Add(
    const EffectObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, IEffectObjectQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t EffectObjectQueue::Process(
    const EffectObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, IEffectObjectQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
