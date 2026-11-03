#include "pal4/heap_trim.h"

namespace pal4 {

bool HeapTrim::Adjust(const std::uint32_t padding,
                      const std::uint32_t heap,
                      IHeapTrimRuntime& runtime) {
    const std::uint32_t top = runtime.Read(heap + 44u);
    const std::uint32_t page = runtime.Read(heap + 860u);
    const std::uint32_t size = runtime.Read(top + 4u) & ~3u;
    const std::uint32_t release =
        (((size - padding + page - 17u) / page) - 1u) * page;
    if (static_cast<std::int32_t>(release) <= 0) {
        return false;
    }
    const std::uint32_t old_end = runtime.ExtendProgramBreak(0u);
    if (old_end != top + size) {
        return false;
    }
    static_cast<void>(runtime.ExtendProgramBreak(0u - release));
    const std::uint32_t new_end = runtime.ExtendProgramBreak(0u);
    if (new_end == 0xffffffffu) {
        return false;
    }
    const std::uint32_t released = old_end - new_end;
    if (released == 0u) {
        return false;
    }
    runtime.Write(heap + 872u, runtime.Read(heap + 872u) - released);
    runtime.Write(top + 4u, (size - released) | 1u);
    return true;
}

}  // namespace pal4
