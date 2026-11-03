#include "pal4/package_archive.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace pal4 {

PackageArchiveState::PackageArchiveState(
    const std::span<std::uint8_t> external_bytes)
    : external_bytes_(external_bytes.data()) {
    if (external_bytes.size() != kBytes)
        throw std::invalid_argument("package archive state size");
}

std::uint8_t* PackageArchiveState::Data() {
    return external_bytes_ ? external_bytes_ : bytes_.data();
}

const std::uint8_t* PackageArchiveState::Data() const {
    return external_bytes_ ? external_bytes_ : bytes_.data();
}

std::uint8_t PackageArchiveState::ReadByte(const std::uint32_t offset) const {
    if (offset >= kBytes) throw std::out_of_range("package archive byte offset");
    return Data()[offset];
}

std::uint32_t PackageArchiveState::ReadWord(const std::uint32_t offset) const {
    if (offset > kBytes - sizeof(std::uint32_t))
        throw std::out_of_range("package archive word offset");
    std::uint32_t value;
    std::memcpy(&value, Data() + offset, sizeof(value));
    return value;
}

void PackageArchiveState::WriteByte(const std::uint32_t offset,
                                     const std::uint8_t value) {
    if (offset >= kBytes) throw std::out_of_range("package archive byte offset");
    Data()[offset] = value;
}

void PackageArchiveState::WriteWord(const std::uint32_t offset,
                                     const std::uint32_t value) {
    if (offset > kBytes - sizeof(value))
        throw std::out_of_range("package archive word offset");
    std::memcpy(Data() + offset, &value, sizeof(value));
}

void PackageArchiveState::Clear(const std::uint32_t offset,
                                 const std::uint32_t count) {
    if (offset > kBytes || count > kBytes - offset)
        throw std::out_of_range("package archive clear range");
    std::fill_n(Data() + offset, count, std::uint8_t{0});
}

PackageArchiveState& PackageArchive::Initialize(
    PackageArchiveState& state, PackageNameCrc& crc,
    IPackageArchiveRuntime& runtime) {
    state.Clear(0u, PackageArchiveState::kBytes);
    state.WriteWord(4u, 2u);
    state.WriteWord(0u, runtime.AllocationGranularity());
    crc.InitializeTable();
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        const std::uint32_t address = runtime.AllocateSlot(48u);
        if (address != 0u) {
            runtime.WriteSlotByte(address, 0u, 0u);
            runtime.WriteSlotWord(address, 32u, 0u);
        }
        state.WriteWord(PackageArchiveState::kSlotArray + index * 4u, address);
        if (address != 0u)
            runtime.WriteSlotByte(address, 0u, 0u);
        else
            runtime.ReportAllocationFailure();
    }
    return state;
}

std::uint32_t PackageArchive::ReleaseSlots(
    PackageArchiveState& state, IPackageArchiveRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        const std::uint32_t offset = PackageArchiveState::kSlotArray + index * 4u;
        result = state.ReadWord(offset);
        if (result != 0u) {
            result = runtime.FreeSlot(result);
            state.WriteWord(offset, 0u);
        }
    }
    return result;
}

std::uint32_t PackageArchive::ResetAfterClose(
    PackageArchiveState& state, IPackageArchiveRuntime& runtime) {
    state.Clear(8u, 0x80u);
    state.Clear(136u, 0x100000u);
    state.WriteByte(PackageArchiveState::kOpenFlag, 0u);
    state.WriteWord(1048748u, 0xffffffffu);
    state.WriteWord(1048752u, 0xffffffffu);
    state.Clear(1048756u, 0x104u);
    state.WriteWord(1049016u, 0u);
    for (std::uint32_t index = 0u; index < 8u; ++index)
        runtime.WriteSlotByte(state.ReadWord(
            PackageArchiveState::kSlotArray + index * 4u), 0u, 0u);
    return PackageArchiveState::kOpenFlag;
}

std::uint8_t PackageArchive::SetModeIfClosed(
    PackageArchiveState& state, const std::uint32_t mode) {
    const std::uint8_t opened = state.ReadByte(PackageArchiveState::kOpenFlag);
    if (opened != 0u) return opened;
    state.WriteWord(4u, mode);
    return static_cast<std::uint8_t>(mode);
}

bool PackageArchive::Close(PackageArchiveState& state,
                           IPackageArchiveFileRuntime& runtime) {
    if (state.ReadByte(PackageArchiveState::kOpenFlag) == 0u) return false;
    if (state.ReadWord(4u) == 2u)
        runtime.CloseMapping(state.ReadWord(1048752u));
    runtime.CloseFile(state.ReadWord(1048748u));
    (void)ResetAfterClose(state, runtime);
    return true;
}

bool PackageArchive::Open(PackageArchiveState& state, const char* path,
                          IPackageArchiveFileRuntime& runtime) {
    constexpr std::uint32_t kPathOffset = 1048756u;
    constexpr std::uint32_t kPathBytes = 0x104u;
    if (path == nullptr) return false;
    std::uint32_t path_length = 0u;
    while (path[path_length] != '\0' && path_length < kPathBytes)
        ++path_length;
    if (path_length >= kPathBytes) return false;
    for (std::uint32_t index = 0u; index <= path_length; ++index)
        state.WriteByte(kPathOffset + index,
                        static_cast<std::uint8_t>(path[index]));

    if (state.ReadByte(PackageArchiveState::kOpenFlag) != 0u) {
        runtime.ReportError(PackageArchiveError::AlreadyLoaded);
        (void)Close(state, runtime);
    }
    const std::uint32_t file = runtime.OpenFile(
        path, 0x80000000u, 1u, 3u, 0x10000080u);
    state.WriteWord(1048748u, file);
    if (file == 0xffffffffu) {
        runtime.ReportError(PackageArchiveError::CannotOpenFile);
        runtime.ReportPathError(PackageArchiveError::CannotOpenFile, path);
        runtime.CloseFile(file);
        return false;
    }

    // The first read's status is ignored by the original; only magic is tested.
    (void)runtime.ReadFile(file, state, 8u, 0x80u);
    if (state.ReadWord(8u) != 0x1a545352u) {
        runtime.ReportError(PackageArchiveError::UnknownFormat);
        runtime.ReportPathError(PackageArchiveError::UnknownFormat, path);
        runtime.CloseFile(file);
        return false;
    }
    const std::uint32_t table_bytes = state.ReadWord(24u) << 5u;
    if (table_bytes > 0x100000u)
        throw std::length_error("package table exceeds archive buffer");
    if (!runtime.ReadFile(file, state, 136u, table_bytes)) {
        runtime.ReportError(PackageArchiveError::CannotLoadTable);
        runtime.ReportPathError(PackageArchiveError::CannotLoadTable, path);
        runtime.CloseFile(file);
        return false;
    }
    runtime.DecryptTable(state, 136u, 0x100000u);
    if (state.ReadWord(4u) == 2u) {
        const std::uint32_t mapping = runtime.CreateMapping(file, 2u);
        state.WriteWord(1048752u, mapping);
        if (mapping == 0u) {
            runtime.ReportError(PackageArchiveError::CannotCreateMapping);
            runtime.ReportPathError(
                PackageArchiveError::CannotCreateMapping, path);
            runtime.CloseFile(file);
            return false;
        }
        if (runtime.LastError() == 183u) {
            runtime.ReportError(PackageArchiveError::MappingAlreadyExists);
            runtime.CloseMapping(mapping);
            state.WriteWord(1048752u, 0u);
            runtime.ReportPathError(
                PackageArchiveError::MappingAlreadyExists, path);
            runtime.CloseFile(file);
            return false;
        }
    }
    state.WriteByte(PackageArchiveState::kOpenFlag, 1u);
    return true;
}

std::uint32_t PackageArchive::FileExistsRaw(
    const PackageArchiveState& state, const char* path,
    IPackageArchiveLookupRuntime& runtime) {
    if (state.ReadByte(PackageArchiveState::kOpenFlag) == 0u)
        return runtime.ReportNoPackageLoaded() & ~0xffu;
    const std::uint32_t index = runtime.FindFileIndex(path);
    return (index & ~0xffu) | static_cast<std::uint32_t>(index != 0xffffffffu);
}

}  // namespace pal4
