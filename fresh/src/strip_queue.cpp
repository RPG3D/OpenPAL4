#include "pal4/strip_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(const StripObjectState& state,
                       const std::uint32_t id,
                       const std::uint32_t opaque,
                       const std::uint32_t slot,
                       IStripQueueRuntime& runtime) {
    EffectSystemHandleWords handle{id, state.words[4u]};
    try {
        runtime.Dispatch(handle.system, slot, handle, opaque);
    } catch (...) {
        (void)runtime.Reset(handle);
        throw;
    }
    return runtime.Reset(handle);
}

}  // namespace

std::uint32_t StripQueue::Add(
    const StripObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, IStripQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t StripQueue::Process(
    const StripObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, IStripQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
