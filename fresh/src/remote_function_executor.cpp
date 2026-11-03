#include "pal4/remote_function_executor.h"

namespace pal4 {

std::uint32_t RemoteFunctionExecutor::Execute(
    const std::uint32_t process_id, const std::uint32_t count,
    const std::uint32_t modules, const std::uint32_t output,
    IRemoteFunctionRuntime& runtime) {
    if (runtime.LoadDebugFunctions() == 0u) {
        runtime.SetLastErrorEx(0x45au, 1u);
        return 0u;
    }

    const std::uint32_t process = runtime.OpenProcess(0x410u, 0u, process_id);
    if (process == 0u) {
        return 0u;
    }
    if (runtime.IsBadCodePointer() == 1u) {
        runtime.CloseHandle(process);
        runtime.SetLastErrorEx(0x482u, 1u);
        return 0u;
    }

    std::uint32_t bytes_written = 0u;
    const std::uint32_t called = runtime.CallDebugFunction(
        process, modules, count * 4u, bytes_written);
    runtime.CloseHandle(process);
    const std::uint32_t modules_written = bytes_written >> 2u;
    runtime.WriteOutput(output, modules_written);

    if ((called == 0u && count != 0u) || bytes_written == 0u) {
        return 0u;
    }
    if (modules_written > count && count != 0u) {
        runtime.SetLastErrorEx(0x7au, 1u);
        return 0u;
    }
    runtime.SetLastError(0u);
    return 1u;
}

}  // namespace pal4
