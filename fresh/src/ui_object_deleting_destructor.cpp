#include "pal4/ui_object_deleting_destructor.h"

namespace pal4 {

std::uint32_t UiObjectDeletingDestructor::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    IUiObjectDeletingRuntime& runtime) {
    runtime.CallBaseRoutine(object);
    if ((flags & 1u) != 0u)
        runtime.Free(object);
    return object;
}

}  // namespace pal4
