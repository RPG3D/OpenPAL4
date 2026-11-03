#include "pal4/sequence_anim_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(
    const SequenceAnimObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, const std::uint32_t slot,
    ISequenceAnimQueueRuntime& runtime) {
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

std::uint32_t SequenceAnimQueue::Add(
    const SequenceAnimObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, ISequenceAnimQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t SequenceAnimQueue::Process(
    const SequenceAnimObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, ISequenceAnimQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
