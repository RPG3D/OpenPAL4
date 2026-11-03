#include "pal4/package_resource_manager.h"

namespace pal4 {

std::uint32_t PackageResourceManager::OpenFile(
    const char* path, IPackageResourceManagerRuntime& runtime) {
    if (path == nullptr) return 0;
    if (!runtime.HasPathRewrite()) return runtime.OpenNativeFile(path);
    std::array<char, 256> rewritten{};
    runtime.RewritePath(path, rewritten);
    rewritten.back() = '\0';
    return runtime.OpenNativeFile(rewritten.data());
}

std::uint8_t PackageResourceManager::FileExists(
    const char* path, IPackageResourceManagerRuntime& runtime) {
    if (path == nullptr) return 0;
    if (!runtime.HasPathRewrite()) return runtime.NativeFileExists(path);
    std::array<char, 256> rewritten{};
    runtime.RewritePath(path, rewritten);
    rewritten.back() = '\0';
    return runtime.NativeFileExists(rewritten.data());
}

}  // namespace pal4
