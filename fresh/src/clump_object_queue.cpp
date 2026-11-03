#include "pal4/clump_object_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(const ClumpObjectState& state,
                       const std::uint32_t id,
                       const std::uint32_t opaque,
                       const std::uint32_t slot,
                       IEffectSystemObjectFQueueRuntime& runtime) {
    EffectSystemHandleWords handle{id, state.words[4u]};
    try {
        const auto result = runtime.Dispatch(handle.system, slot, handle,
                                             opaque);
        runtime.Reset(handle);
        return result;
    } catch (...) {
        runtime.Reset(handle);
        throw;
    }
}

}  // namespace

std::uint32_t ClumpObjectQueue::Add(
    const ClumpObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque,
    IEffectSystemObjectFQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t ClumpObjectQueue::Process(
    const ClumpObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque,
    IEffectSystemObjectFQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
