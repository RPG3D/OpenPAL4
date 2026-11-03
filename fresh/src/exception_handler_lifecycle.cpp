#include "pal4/exception_handler_lifecycle.h"

namespace pal4 {

void ExceptionHandlerLifecycle::InitializeEmpty() {}

std::uint32_t ExceptionHandlerLifecycle::InitializeAndRegisterExit(
    IExceptionHandlerRuntime& runtime) {
    InitializeEmpty();
    return runtime.RegisterExitHandler(kCleanupEntry);
}

void ExceptionHandlerLifecycle::Cleanup(IExceptionHandlerRuntime& runtime) {
    const std::uint32_t buffer = runtime.Read(kBuffer);
    if (buffer != 0u) {
        (void)runtime.HeapFree(runtime.GetProcessHeap(), 0u, buffer);
        runtime.Write(kBuffer, 0u);
    }
    const std::uint32_t previous = runtime.Read(kPreviousFilter);
    if (previous != 0u) {
        runtime.SetUnhandledExceptionFilter(previous);
    }
}

}  // namespace pal4
