#pragma once

#include <cstddef>
#include <cstdint>

#include "pal4/package_archive.h"
#include "pal4/package_file_buffer.h"

namespace pal4 {

struct PackageArchiveFileSlot final {
    std::uint8_t active = 0;
    std::uint8_t reserved0[3]{};
    std::uint32_t file_id = 0;
    std::uint32_t logical_bytes = 0;
    std::uint32_t index = 0;
    std::uint32_t mapped_base = 0;
    std::uint32_t cursor = 0;
    std::uint32_t mapping_delta = 0;
    std::uint8_t needs_decode = 0;
    std::uint8_t reserved28[3]{};
    std::uint32_t data = 0;
    std::uint32_t decoded_bytes = 0;
    std::uint32_t reserved40 = 0;
    std::uint32_t record_offset = 0;
};
static_assert(sizeof(PackageArchiveFileSlot) == 48u);
static_assert(offsetof(PackageArchiveFileSlot, mapped_base) == 16u);
static_assert(offsetof(PackageArchiveFileSlot, needs_decode) == 28u);
static_assert(offsetof(PackageArchiveFileSlot, record_offset) == 44u);

enum class PackageArchiveEntryError {
    TooManyOpenFiles,
    CannotMapFile,
    CannotAllocateDecoded,
    CannotDecompress,
    InvalidMappedAddress,
    CannotUnmapFile,
};

class IPackageArchiveEntryRuntime {
public:
    virtual ~IPackageArchiveEntryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindIndex(const char* path) = 0;
    [[nodiscard]] virtual PackageArchiveFileSlot* Slot(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t MapView(
        std::uint32_t mapping, std::uint32_t access,
        std::uint32_t aligned_offset, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual bool UnmapView(std::uint32_t base) = 0;
    virtual void EnsureDecoderAllocator() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateDecoded(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual bool Decode(std::uint16_t type,
                                      std::uint32_t packed_bytes,
                                      std::uint32_t decoded_bytes,
                                      std::uint32_t source,
                                      std::uint32_t destination) = 0;
    virtual void FreeDecoded(std::uint32_t address,
                             std::uint32_t bytes) = 0;
    virtual void SetFilePointer(std::uint32_t file,
                                std::uint32_t offset) = 0;
    virtual void ReportError(PackageArchiveEntryError error) = 0;
    virtual void ReportMapFailure(const char* file_path,
                                  const char* member_path,
                                  std::uint32_t aligned_offset,
                                  std::uint32_t bytes) = 0;
};

class IPackageArchiveReadRuntime {
public:
    virtual ~IPackageArchiveReadRuntime() = default;
    virtual void CopyFromAddress(void* destination,
                                 std::uint32_t source,
                                 std::uint32_t bytes) = 0;
    virtual void ReadFile(std::uint32_t file, void* destination,
                          std::uint32_t bytes) = 0;
};

class PackageArchiveFileEntry final {
public:
    // IDA 0x7938A0. Returns the selected native slot address or zero.
    [[nodiscard]] static std::uint32_t Open(
        PackageArchiveState& archive, const char* path,
        IPackageArchiveEntryRuntime& runtime);
    // IDA 0x793C10.
    [[nodiscard]] static bool Close(PackageArchiveState& archive,
                                    std::uint32_t slot_address,
                                    IPackageArchiveEntryRuntime& runtime);
    // IDA 0x793CE0: preserves upper cursor bytes while forcing AL to one.
    [[nodiscard]] static std::uint32_t ReadData(
        const PackageArchiveState& archive, PackageArchiveFileSlot& slot,
        void* destination, std::uint32_t bytes,
        IPackageArchiveReadRuntime& runtime);
    // IDA 0x793D50. Origin 2 subtracts the offset from decoded_bytes.
    [[nodiscard]] static std::uint32_t Seek(
        const PackageArchiveState& archive, PackageArchiveFileSlot& slot,
        std::int32_t offset, std::uint32_t origin,
        IPackageArchiveEntryRuntime& runtime);
    // IDA 0x793DC0.
    static void LoadToBuffer(const PackageArchiveState& archive,
                             const PackageArchiveFileSlot* slot,
                             PackageFileBufferState* buffer,
                             IPackageFileBufferRuntime& runtime);
};

}  // namespace pal4
