#include "pal4/end_attack_child_task.h"

namespace pal4 {

std::uint32_t EndAttackBaseChildTask::Create(
    IEndAttackChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
        runtime.WriteWord(object, kVtable);
        return object;
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

}  // namespace pal4
