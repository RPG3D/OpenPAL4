#include "pal4/exception_handler_pointer_array.h"

namespace pal4 {

std::uint32_t ExceptionHandlerPointerArray::Append(
    const std::uint32_t element,
    IExceptionHandlerPointerArrayRuntime& runtime) {
    if (element == 0u) {
        return 0u;
    }
    const std::uint32_t bytes = 4u * runtime.Read(kCount) + 4u;
    const std::uint32_t replacement = runtime.HeapAllocate(
        runtime.GetProcessHeap(), 0xcu, bytes);
    if (replacement == 0u) {
        return 0u;
    }

    const std::uint32_t previous = runtime.Read(kBuffer);
    if (previous != 0u) {
        runtime.CopyDwords(replacement, previous, runtime.Read(kCount));
        (void)runtime.HeapFree(runtime.GetProcessHeap(), 0u, previous);
        runtime.Write(kBuffer, replacement);
        runtime.Write(replacement + runtime.Read(kCount) * 4u, element);
    } else {
        runtime.Write(kBuffer, replacement);
        runtime.Write(replacement, element);
    }
    runtime.Write(kCount, runtime.Read(kCount) + 1u);
    return 1u;
}

}  // namespace pal4
