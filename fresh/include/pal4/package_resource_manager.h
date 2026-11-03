#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IPackageResourceManagerRuntime {
public:
    virtual ~IPackageResourceManagerRuntime() = default;
    [[nodiscard]] virtual bool HasPathRewrite() = 0;
    virtual void RewritePath(const char* source,
                             std::array<char, 256>& output) = 0;
    [[nodiscard]] virtual std::uint32_t OpenNativeFile(
        const char* path) = 0;
    [[nodiscard]] virtual std::uint8_t NativeFileExists(
        const char* path) = 0;
};

class PackageResourceManagerRuntimeStub final
    : public IPackageResourceManagerRuntime {
public:
    [[nodiscard]] bool HasPathRewrite() override { return false; }
    void RewritePath(const char*, std::array<char, 256>&) override {}
    [[nodiscard]] std::uint32_t OpenNativeFile(
        const char*) override { return 0; }
    [[nodiscard]] std::uint8_t NativeFileExists(
        const char*) override { return 0; }
};

class PackageResourceManager final {
public:
    [[nodiscard]] static std::uint32_t OpenFile(
        const char* path, IPackageResourceManagerRuntime& runtime);
    [[nodiscard]] static std::uint8_t FileExists(
        const char* path, IPackageResourceManagerRuntime& runtime);
};

}  // namespace pal4
