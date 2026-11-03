#include "pal4/package_disc_file_api.h"

namespace pal4 {
namespace {
[[nodiscard]] std::uint32_t WithLowByte(std::uint32_t raw,
                                         std::uint8_t low) {
    return (raw & ~0xffu) | low;
}
}  // namespace

std::uint32_t PackageDiscFileApi::AllocationGranularity() {
    return win32_.AllocationGranularity();
}

std::uint32_t PackageDiscFileApi::CreateFile(
    const char* path, std::uint32_t access, std::uint32_t share,
    std::uint32_t security, std::uint32_t disposition,
    std::uint32_t attributes, std::uint32_t template_file) {
    if (mode_ != 4u) return 0u;
    return win32_.CreateFile(path, access, share, security, disposition,
                             attributes, template_file);
}

std::uint32_t PackageDiscFileApi::CreateMapping(
    std::uint32_t file, std::uint32_t security, std::uint32_t protection,
    std::uint32_t maximum_low, const char* name) {
    if (mode_ != 4u) return 0u;
    return win32_.CreateMapping(file, security, protection, 0u,
                                maximum_low, name);
}

std::uint32_t PackageDiscFileApi::CloseFileHandle(std::uint32_t handle) {
    if (mode_ != 4u) return 0u;
    return win32_.CloseHandle(handle);
}

std::uint32_t PackageDiscFileApi::MapView(
    std::uint32_t mapping, std::uint32_t access,
    std::uint32_t offset_low, std::uint32_t bytes) {
    if (mode_ != 4u) return 0u;
    return win32_.MapView(mapping, access, 0u, offset_low, bytes);
}

std::uint32_t PackageDiscFileApi::UnmapView(
    std::uint32_t address, std::uint32_t incoming_eax) {
    if (mode_ != 4u) return WithLowByte(incoming_eax, 0u);
    return win32_.UnmapView(address) == 1u ? 1u : 0u;
}

std::uint32_t PackageDiscFileApi::ReadFile(
    std::uint32_t file, void* destination, std::uint32_t bytes,
    std::uint32_t incoming_eax) {
    if (mode_ != 4u) return WithLowByte(incoming_eax, 0u);
    const auto result = win32_.ReadFile(file, destination, bytes);
    last_bytes_read_ = result.bytes_read;
    last_read_status_byte_ = result.success == 1u ? 1u : 0u;
    const std::uint32_t raw = WithLowByte(result.success,
                                         last_read_status_byte_);
    if (result.bytes_read == bytes) return raw;
    return WithLowByte(win32_.ReportShortRead(), 0u);
}

std::uint32_t PackageDiscFileApi::CloseMappingHandle(
    std::uint32_t handle, std::uint32_t incoming_eax) {
    if (mode_ != 4u) return WithLowByte(incoming_eax, 0u);
    return win32_.CloseHandle(handle) == 1u ? 1u : 0u;
}

std::uint32_t PackageDiscFileApi::SetFilePointer(
    std::uint32_t file, std::int32_t distance) {
    if (mode_ != 4u) return 0u;
    return win32_.SetFilePointer(file, distance, 0u);
}

std::uint32_t PackageDiscFileApi::FileExists(
    const char* path, std::uint32_t incoming_eax) {
    if (mode_ != 4u) return WithLowByte(incoming_eax, 0u);
    const std::uint32_t file = win32_.CreateFile(
        path, 0x80000000u, 1u, 0u, 3u, 0x10000080u, 0u);
    if (file == 0xffffffffu) return WithLowByte(file, 0u);
    return WithLowByte(win32_.CloseHandle(file), 1u);
}

}  // namespace pal4
