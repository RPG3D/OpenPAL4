#include "pal4/gob_state_apply.h"

namespace pal4 {

std::uint32_t GobStateApply::InvokeTypeIfStored(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobStateApplyRuntime& runtime) {
    const auto stored = runtime.ReadWord(object + 0x1a4u);
    if (stored == 0u) return stored;
    return runtime.InvokeSnapshotTypeId(snapshot);
}

}  // namespace pal4
