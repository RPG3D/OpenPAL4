#include "pal4/combat_defend_status_child_task.h"

namespace pal4 {

std::uint32_t DefendStatusChildTask::Create(
    IDefendStatusChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    try {
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.DestroyBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

}  // namespace pal4
