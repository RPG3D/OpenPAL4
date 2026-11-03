#include "pal4/sound3d_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(
    const Sound3DObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, const std::uint32_t slot,
    ISound3DQueueRuntime& runtime) {
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

std::uint32_t Sound3DQueue::Add(
    const Sound3DObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, ISound3DQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t Sound3DQueue::Process(
    const Sound3DObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, ISound3DQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
