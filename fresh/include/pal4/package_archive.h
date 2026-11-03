#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "pal4/package_name_crc.h"

namespace pal4 {

class PackageArchiveState final {
public:
    static constexpr std::uint32_t kBytes = 0x1001bcu;
    static constexpr std::uint32_t kSlotArray = 1048712u;
    static constexpr std::uint32_t kOpenFlag = 1048744u;

    PackageArchiveState() : bytes_(kBytes) {}
    explicit PackageArchiveState(std::span<std::uint8_t> external_bytes);
    PackageArchiveState(const PackageArchiveState&) = delete;
    PackageArchiveState& operator=(const PackageArchiveState&) = delete;
    PackageArchiveState(PackageArchiveState&&) = delete;
    PackageArchiveState& operator=(PackageArchiveState&&) = delete;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t offset) const;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t offset) const;
    void WriteByte(std::uint32_t offset, std::uint8_t value);
    void WriteWord(std::uint32_t offset, std::uint32_t value);
    void Clear(std::uint32_t offset, std::uint32_t count);

private:
    [[nodiscard]] std::uint8_t* Data();
    [[nodiscard]] const std::uint8_t* Data() const;
    std::vector<std::uint8_t> bytes_;
    std::uint8_t* external_bytes_ = nullptr;
};

class IPackageArchiveRuntime {
public:
    virtual ~IPackageArchiveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocationGranularity() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateSlot(std::uint32_t bytes) = 0;
    virtual void WriteSlotByte(std::uint32_t address, std::uint32_t offset,
                               std::uint8_t value) = 0;
    virtual void WriteSlotWord(std::uint32_t address, std::uint32_t offset,
                               std::uint32_t value) = 0;
    virtual void ReportAllocationFailure() = 0;
    [[nodiscard]] virtual std::uint32_t FreeSlot(std::uint32_t address) = 0;
};

enum class PackageArchiveError {
    AlreadyLoaded,
    CannotOpenFile,
    UnknownFormat,
    CannotLoadTable,
    CannotCreateMapping,
    MappingAlreadyExists,
};

class IPackageArchiveFileRuntime : public IPackageArchiveRuntime {
public:
    [[nodiscard]] virtual std::uint32_t OpenFile(
        const char* path, std::uint32_t access, std::uint32_t share,
        std::uint32_t disposition, std::uint32_t attributes) = 0;
    [[nodiscard]] virtual bool ReadFile(std::uint32_t handle,
                                        PackageArchiveState& state,
                                        std::uint32_t offset,
                                        std::uint32_t bytes) = 0;
    virtual void DecryptTable(PackageArchiveState& state,
                              std::uint32_t offset,
                              std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CreateMapping(
        std::uint32_t file, std::uint32_t protection) = 0;
    [[nodiscard]] virtual std::uint32_t LastError() = 0;
    virtual void CloseFile(std::uint32_t handle) = 0;
    virtual void CloseMapping(std::uint32_t handle) = 0;
    virtual void ReportError(PackageArchiveError error) = 0;
    virtual void ReportPathError(PackageArchiveError error,
                                 const char* path) = 0;
};

class IPackageArchiveLookupRuntime {
public:
    virtual ~IPackageArchiveLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindFileIndex(const char* path) = 0;
    [[nodiscard]] virtual std::uint32_t ReportNoPackageLoaded() = 0;
};

class PackageArchive final {
public:
    // IDA 0x7934C0.
    [[nodiscard]] static PackageArchiveState& Initialize(
        PackageArchiveState& state, PackageNameCrc& crc,
        IPackageArchiveRuntime& runtime);
    // IDA 0x793540.
    [[nodiscard]] static std::uint32_t ReleaseSlots(
        PackageArchiveState& state, IPackageArchiveRuntime& runtime);
    // IDA 0x793570. Returns the original one-past-slot array offset.
    [[nodiscard]] static std::uint32_t ResetAfterClose(
        PackageArchiveState& state, IPackageArchiveRuntime& runtime);
    // IDA 0x7935D0.
    [[nodiscard]] static std::uint8_t SetModeIfClosed(
        PackageArchiveState& state, std::uint32_t mode);
    // IDA 0x7935F0.
    [[nodiscard]] static bool Open(PackageArchiveState& state,
                                   const char* path,
                                   IPackageArchiveFileRuntime& runtime);
    // IDA 0x793820.
    [[nodiscard]] static bool Close(PackageArchiveState& state,
                                    IPackageArchiveFileRuntime& runtime);
    // IDA 0x793860. The original changes AL only, retaining the high bytes.
    [[nodiscard]] static std::uint32_t FileExistsRaw(
        const PackageArchiveState& state, const char* path,
        IPackageArchiveLookupRuntime& runtime);
};

}  // namespace pal4
