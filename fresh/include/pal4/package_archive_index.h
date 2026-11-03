#pragma once

#include <cstdint>

#include "pal4/package_archive.h"

namespace pal4 {

class IPackageArchiveIndexRuntime {
public:
    virtual ~IPackageArchiveIndexRuntime() = default;
    virtual void SetFilePointer(std::uint32_t file, std::uint32_t offset) = 0;
    virtual void ReadFile(std::uint32_t file, void* destination,
                          std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReportMissingDirectory(
        const char* path) = 0;
};

using PackageFileVisitor = int (*)(const char* name, void* context);

class PackageArchiveIndex final {
public:
    // IDA 0x793F40: binary search over 32-byte CPK records.
    [[nodiscard]] static std::int32_t FindByHash(
        const PackageArchiveState& archive, std::uint32_t hash);
    // IDA 0x793EF0: lowercase the path before the package CRC lookup.
    [[nodiscard]] static std::int32_t FindByName(
        const PackageArchiveState& archive, const char* path,
        const PackageNameCrc& crc);
    // IDA 0x793E10: enumerate child file names of an indexed directory.
    [[nodiscard]] static std::uint32_t EnumerateFiles(
        const PackageArchiveState& archive, const char* directory,
        const PackageNameCrc& crc, PackageFileVisitor visitor,
        void* context, IPackageArchiveIndexRuntime& runtime);
};

}  // namespace pal4
