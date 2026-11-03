#pragma once

#include <cstdint>

namespace pal4 {

struct PackageDiscReadResult final {
    std::uint32_t success;
    std::uint32_t bytes_read;
};

class IPackageDiscWin32Api {
public:
    virtual ~IPackageDiscWin32Api() = default;
    [[nodiscard]] virtual std::uint32_t AllocationGranularity() = 0;
    [[nodiscard]] virtual std::uint32_t CreateFile(
        const char* path, std::uint32_t access, std::uint32_t share,
        std::uint32_t security, std::uint32_t disposition,
        std::uint32_t attributes, std::uint32_t template_file) = 0;
    [[nodiscard]] virtual std::uint32_t CreateMapping(
        std::uint32_t file, std::uint32_t security, std::uint32_t protection,
        std::uint32_t maximum_high, std::uint32_t maximum_low,
        const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t CloseHandle(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t MapView(
        std::uint32_t mapping, std::uint32_t access,
        std::uint32_t offset_high, std::uint32_t offset_low,
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t UnmapView(std::uint32_t address) = 0;
    [[nodiscard]] virtual PackageDiscReadResult ReadFile(
        std::uint32_t file, void* destination, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SetFilePointer(
        std::uint32_t file, std::int32_t distance,
        std::uint32_t method) = 0;
    [[nodiscard]] virtual std::uint32_t ReportShortRead() = 0;
};

class PackageDiscFileApi final {
public:
    explicit PackageDiscFileApi(IPackageDiscWin32Api& win32,
                                std::uint32_t mode = 4u)
        : win32_(win32), mode_(mode) {}

    // IDA 0x793FA0.
    [[nodiscard]] std::uint32_t AllocationGranularity();
    // IDA 0x793FC0.
    [[nodiscard]] std::uint32_t CreateFile(
        const char* path, std::uint32_t access, std::uint32_t share,
        std::uint32_t security, std::uint32_t disposition,
        std::uint32_t attributes, std::uint32_t template_file);
    // IDA 0x794000.
    [[nodiscard]] std::uint32_t CreateMapping(
        std::uint32_t file, std::uint32_t security,
        std::uint32_t protection, std::uint32_t maximum_low,
        const char* name);
    // IDA 0x794030: returns the Win32 BOOL without normalizing it.
    [[nodiscard]] std::uint32_t CloseFileHandle(std::uint32_t handle);
    // IDA 0x794050.
    [[nodiscard]] std::uint32_t MapView(
        std::uint32_t mapping, std::uint32_t access,
        std::uint32_t offset_low, std::uint32_t bytes);
    // IDA 0x794080: normalizes a Win32 BOOL equal to one.
    [[nodiscard]] std::uint32_t UnmapView(std::uint32_t address,
                                          std::uint32_t incoming_eax = 0u);
    // IDA 0x7940A0: checks the byte count independently of API success.
    [[nodiscard]] std::uint32_t ReadFile(
        std::uint32_t file, void* destination, std::uint32_t bytes,
        std::uint32_t incoming_eax = 0u);
    // IDA 0x794100: second close wrapper normalizes BOOL equal to one.
    [[nodiscard]] std::uint32_t CloseMappingHandle(
        std::uint32_t handle, std::uint32_t incoming_eax = 0u);
    // IDA 0x794120.
    [[nodiscard]] std::uint32_t SetFilePointer(
        std::uint32_t file, std::int32_t distance);
    // IDA 0x794150: returns the original incidental upper EAX bytes.
    [[nodiscard]] std::uint32_t FileExists(
        const char* path, std::uint32_t incoming_eax = 0u);

    [[nodiscard]] std::uint8_t LastReadStatusByte() const noexcept {
        return last_read_status_byte_;
    }
    [[nodiscard]] std::uint32_t LastBytesRead() const noexcept {
        return last_bytes_read_;
    }

private:
    IPackageDiscWin32Api& win32_;
    std::uint32_t mode_;
    std::uint8_t last_read_status_byte_ = 0xffu;
    std::uint32_t last_bytes_read_ = 0u;
};

}  // namespace pal4
