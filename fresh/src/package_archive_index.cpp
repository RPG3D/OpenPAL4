#include "pal4/package_archive_index.h"

#include <array>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBase = 136u;
constexpr std::uint32_t kRecordBytes = 32u;

[[nodiscard]] std::uint32_t RecordOffset(std::uint32_t index) {
    return kRecordBase + index * kRecordBytes;
}

}  // namespace

std::int32_t PackageArchiveIndex::FindByHash(
    const PackageArchiveState& archive, const std::uint32_t hash) {
    std::int32_t low = 0;
    std::int32_t high = static_cast<std::int32_t>(archive.ReadWord(40u));
    if (high < 0 || high > 0x100000 / static_cast<std::int32_t>(kRecordBytes))
        throw std::out_of_range("package index count");
    while (high != low) {
        const std::int32_t middle = low + (high - low) / 2;
        const std::uint32_t record = RecordOffset(
            static_cast<std::uint32_t>(middle));
        const std::uint32_t current = archive.ReadWord(record);
        const std::uint32_t flags = archive.ReadWord(record + 4u);
        if (hash == current && (flags & 1u) != 0u &&
            (flags & 0x10u) == 0u)
            return middle;
        if (high == low + 1) return -1;
        if (hash < current)
            high = middle;
        else
            low = middle;
    }
    return -1;
}

std::int32_t PackageArchiveIndex::FindByName(
    const PackageArchiveState& archive, const char* path,
    const PackageNameCrc& crc) {
    if (path == nullptr) throw std::invalid_argument("package path");
    std::array<std::uint8_t, 324> lower{};
    const std::size_t length = std::strlen(path);
    if (length >= lower.size()) throw std::length_error("package path");
    for (std::size_t i = 0; i < length; ++i)
        lower[i] = static_cast<std::uint8_t>(std::tolower(
            static_cast<unsigned char>(path[i])));
    return FindByHash(archive, crc.Compute(lower.data()));
}

std::uint32_t PackageArchiveIndex::EnumerateFiles(
    const PackageArchiveState& archive, const char* directory,
    const PackageNameCrc& crc, const PackageFileVisitor visitor,
    void* const context, IPackageArchiveIndexRuntime& runtime) {
    const std::int32_t directory_index = FindByName(archive, directory, crc);
    if (directory_index == -1)
        return runtime.ReportMissingDirectory(directory);
    const std::uint32_t directory_hash = archive.ReadWord(
        RecordOffset(static_cast<std::uint32_t>(directory_index)));
    const std::uint32_t count = archive.ReadWord(40u);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint32_t record = RecordOffset(i);
        const std::uint32_t flags = archive.ReadWord(record + 4u);
        if (archive.ReadWord(record + 8u) != directory_hash ||
            (flags & 1u) == 0u || (flags & 0x12u) != 0u)
            continue;
        const std::uint32_t bytes = archive.ReadWord(record + 24u);
        std::array<char, 324> name{};
        if (bytes >= name.size()) throw std::length_error("package file name");
        const std::uint32_t file = archive.ReadWord(1048748u);
        runtime.SetFilePointer(file, archive.ReadWord(record + 12u) +
                                    archive.ReadWord(record + 16u));
        runtime.ReadFile(file, name.data(), bytes);
        if (visitor != nullptr && visitor(name.data(), context) == 0u)
            return 0u;
    }
    return count;
}

}  // namespace pal4
