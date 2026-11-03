#include "pal4/process_heap_finalizer.h"

namespace pal4 {

std::uint32_t ProcessHeapFinalizer::Finalize(
    IProcessHeapFinalizerRuntime& runtime) {
    std::uint32_t node = head_;
    if (node == 0u) {
        return 0u;
    }
    for (std::uint32_t next = runtime.ReadNext(node); next != 0u;
         next = runtime.ReadNext(node)) {
        const std::uint32_t successor = next;
        const std::uint32_t result =
            runtime.FreeFromHeap(runtime.GetProcessHeap(), node);
        if (result == 0u) {
            return 0u;
        }
        node = successor;
        head_ = node;
    }
    return runtime.FreeFromHeap(runtime.GetProcessHeap(), node);
}

}  // namespace pal4
