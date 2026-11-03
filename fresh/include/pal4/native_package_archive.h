#pragma once

#include "pal4/package_archive.h"
#include "pal4/package_archive_index.h"
#include "pal4/package_archive_file_entry.h"

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pal4 {

class NativePackageArchive final : private IPackageArchiveFileRuntime,
                                   private IPackageArchiveIndexRuntime,
                                   private IPackageArchiveEntryRuntime,
                                   private IPackageArchiveReadRuntime {
public:
    NativePackageArchive();
    // Borrowed storage must outlive this archive.
    explicit NativePackageArchive(std::span<std::uint8_t> state_bytes);
    ~NativePackageArchive();
    NativePackageArchive(const NativePackageArchive&) = delete;
    NativePackageArchive& operator=(const NativePackageArchive&) = delete;

    [[nodiscard]] bool Open(const char* path);
    [[nodiscard]] bool Close();
    [[nodiscard]] std::int32_t Find(const char* path) const;
    [[nodiscard]] std::vector<std::string> ListFiles(const char* directory);
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadMember(
        const char* path);
    // Mirrors Package_LoadFile: decrypt members marked by the CPK entry.
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadLoadedMember(
        const char* path);
    [[nodiscard]] std::uint32_t OpenMember(const char* path);
    [[nodiscard]] bool OwnsMemberHandle(std::uint32_t handle) const;
    [[nodiscard]] bool CloseMember(std::uint32_t handle);
    [[nodiscard]] std::uint32_t ReadMemberData(
        std::uint32_t handle, void* destination, std::uint32_t bytes);
    [[nodiscard]] std::int32_t SeekMember(
        std::uint32_t handle, std::int32_t offset, std::uint32_t origin);
    [[nodiscard]] std::uint32_t EntryCount() const { return state_.ReadWord(40u); }
    [[nodiscard]] std::optional<PackageArchiveError> LastFailure() const {
        return last_failure_;
    }

private:
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadMemberImpl(
        const char* path, bool decrypt_entry);
    void Initialize();
    std::uint32_t AllocationGranularity() override;
    std::uint32_t AllocateSlot(std::uint32_t bytes) override;
    void WriteSlotByte(std::uint32_t address, std::uint32_t offset,
                       std::uint8_t value) override;
    void WriteSlotWord(std::uint32_t address, std::uint32_t offset,
                       std::uint32_t value) override;
    void ReportAllocationFailure() override;
    std::uint32_t FreeSlot(std::uint32_t address) override;
    std::uint32_t OpenFile(const char* path, std::uint32_t access,
                           std::uint32_t share, std::uint32_t disposition,
                           std::uint32_t attributes) override;
    bool ReadFile(std::uint32_t handle, PackageArchiveState& state,
                  std::uint32_t offset, std::uint32_t bytes) override;
    void DecryptTable(PackageArchiveState& state, std::uint32_t offset,
                      std::uint32_t bytes) override;
    std::uint32_t CreateMapping(std::uint32_t file,
                                std::uint32_t protection) override;
    std::uint32_t LastError() override;
    void CloseFile(std::uint32_t handle) override;
    void CloseMapping(std::uint32_t handle) override;
    void ReportError(PackageArchiveError error) override;
    void ReportPathError(PackageArchiveError error,
                         const char* path) override;
    void SetFilePointer(std::uint32_t file, std::uint32_t offset) override;
    void ReadFile(std::uint32_t file, void* destination,
                  std::uint32_t bytes) override;
    std::uint32_t ReportMissingDirectory(const char* path) override;
    std::uint32_t FindIndex(const char* path) override;
    PackageArchiveFileSlot* Slot(std::uint32_t address) override;
    std::uint32_t MapView(std::uint32_t mapping, std::uint32_t access,
                          std::uint32_t aligned_offset,
                          std::uint32_t bytes) override;
    bool UnmapView(std::uint32_t base) override;
    void EnsureDecoderAllocator() override;
    std::uint32_t AllocateDecoded(std::uint32_t bytes) override;
    bool Decode(std::uint16_t type, std::uint32_t packed_bytes,
                std::uint32_t decoded_bytes, std::uint32_t source,
                std::uint32_t destination) override;
    void FreeDecoded(std::uint32_t address, std::uint32_t bytes) override;
    void ReportError(PackageArchiveEntryError error) override;
    void ReportMapFailure(const char* file_path, const char* member_path,
                          std::uint32_t aligned_offset,
                          std::uint32_t bytes) override;
    void CopyFromAddress(void* destination, std::uint32_t source,
                         std::uint32_t bytes) override;

    PackageArchiveState state_;
    PackageNameCrc crc_;
    std::optional<PackageArchiveError> last_failure_;
    bool allocation_failed_ = false;
};

}  // namespace pal4
