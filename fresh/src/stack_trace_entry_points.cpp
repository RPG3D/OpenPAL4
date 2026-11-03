#include "pal4/stack_trace_entry_points.h"

namespace pal4 {

std::uint32_t StackTraceEntryPoints::Prepare(
    const std::uint32_t first_arg,
    const std::uint32_t exception_pointers,
    IStackTraceEntryRuntime& runtime) {
    for (std::uint32_t offset = 0u; offset < kFrameBytes; offset += 4u) {
        runtime.Write(kFrameAddress + offset, 0u);
    }
    const std::uint32_t context = runtime.Read(exception_pointers + 4u);
    runtime.Write(kFrameAddress, runtime.Read(context + 184u));
    runtime.Write(kFrameAddress + 8u, 3u);
    runtime.Write(kFrameAddress + 36u, runtime.Read(context + 196u));
    runtime.Write(kFrameAddress + 44u, 3u);
    runtime.Write(kFrameAddress + 24u, runtime.Read(context + 180u));
    runtime.Write(kFrameAddress + 32u, 3u);
    return runtime.GenerateStackTraceString(first_arg, exception_pointers);
}

std::uint32_t StackTraceEntryPoints::Forward(
    const std::uint32_t first_arg,
    const std::uint32_t exception_pointers,
    IStackTraceEntryRuntime& runtime) {
    return runtime.GenerateStackTraceString(first_arg, exception_pointers);
}

std::uint32_t StackTraceEntryPoints::ReadCurrentProcessMemory(
    const std::uint32_t, const std::uint32_t source,
    const std::uint32_t destination, const std::uint32_t bytes,
    const std::uint32_t bytes_read_pointer,
    IStackTraceEntryRuntime& runtime) {
    return runtime.ReadProcessMemory(runtime.GetCurrentProcess(), source,
                                     destination, bytes, bytes_read_pointer);
}

}  // namespace pal4
