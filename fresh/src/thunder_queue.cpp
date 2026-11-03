#include "pal4/thunder_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(const ThunderObjectState& state, const std::uint32_t id,
                       const std::uint32_t opaque, const std::uint32_t slot,
                       IThunderQueueRuntime& runtime) {
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

std::uint32_t ThunderQueue::Add(
    const ThunderObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, IThunderQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t ThunderQueue::Process(
    const ThunderObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, IThunderQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
