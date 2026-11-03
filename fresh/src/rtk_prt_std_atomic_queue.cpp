#include "pal4/rtk_prt_std_atomic_queue.h"

namespace pal4 {
namespace {

std::uint32_t Dispatch(
    const RtkPrtStdAtomicObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque, const std::uint32_t slot,
    IRtkPrtStdAtomicQueueRuntime& runtime) {
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

std::uint32_t RtkPrtStdAtomicQueue::Add(
    const RtkPrtStdAtomicObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque,
    IRtkPrtStdAtomicQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 12u, runtime);
}

std::uint32_t RtkPrtStdAtomicQueue::Process(
    const RtkPrtStdAtomicObjectState& state, const std::uint32_t id,
    const std::uint32_t opaque,
    IRtkPrtStdAtomicQueueRuntime& runtime) {
    return Dispatch(state, id, opaque, 16u, runtime);
}

}  // namespace pal4
