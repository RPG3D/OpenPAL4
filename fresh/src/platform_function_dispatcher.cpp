#include "pal4/platform_function_dispatcher.h"

namespace pal4 {

std::uint32_t PlatformFunctionDispatcher::Dispatch(
    const std::uint32_t process_id, const std::uint32_t count,
    const std::uint32_t modules, const std::uint32_t output,
    IPlatformFunctionRuntime& runtime) {
    if (runtime.IsBadWritePointer(output, 4u) == 1u ||
        (count != 0u &&
         runtime.IsBadWritePointer(modules, count * 4u) == 1u)) {
        runtime.SetLastErrorEx(0x57u, 1u);
        return 0u;
    }

    OperatingSystemVersion version{};
    version.size = sizeof(version);
    if (runtime.GetVersion(version) == 0u) {
        return 0u;
    }
    if (version.platform == 2u && version.major == 4u) {
        return runtime.ExecuteRemoteFunction(process_id, count, modules, output);
    }
    return runtime.EnumerateProcessModules(process_id, count, modules, output);
}

std::uint32_t OperatingSystemPlatformCache::IsNt(
    IPlatformFunctionRuntime& runtime) {
    if (guard_ == 1u) {
        return is_nt_ == 1u ? 1u : 0u;
    }
    OperatingSystemVersion version{};
    version.size = sizeof(version);
    if (runtime.GetVersion(version) == 0u) {
        return 0u;
    }
    guard_ = 1u;
    is_nt_ = version.platform == 2u ? 1u : 0u;
    return is_nt_;
}

}  // namespace pal4
