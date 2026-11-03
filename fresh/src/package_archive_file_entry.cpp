#include "pal4/package_archive_file_entry.h"

#include <stdexcept>
#include <string>

namespace pal4 {

std::uint32_t PackageArchiveFileEntry::Open(
    PackageArchiveState& archive, const char* path,
    IPackageArchiveEntryRuntime& runtime) {
    if (archive.ReadByte(PackageArchiveState::kOpenFlag) == 0u) return 0u;
    if (static_cast<std::int32_t>(archive.ReadWord(1049016u)) >= 7) {
        runtime.ReportError(PackageArchiveEntryError::TooManyOpenFiles);
        return 0u;
    }
    const std::uint32_t index = runtime.FindIndex(path);
    if (index == 0xffffffffu) return 0u;
    if (index >= 0x100000u / 32u)
        throw std::out_of_range("package file index exceeds table");
    const std::uint32_t record = 136u + (index << 5u);

    const std::uint32_t offset = archive.ReadWord(record + 12u);
    const bool mapped = archive.ReadWord(4u) == 2u;
    const std::uint32_t granularity = archive.ReadWord(0u);
    if (mapped && granularity == 0u)
        throw std::domain_error("zero package mapping granularity");
    const std::uint32_t aligned = mapped
        ? offset - offset % granularity : offset;
    const std::uint32_t delta = offset - aligned;
    const std::uint32_t mapping_bytes = delta +
        archive.ReadWord(record + 16u) + archive.ReadWord(record + 24u);

    std::uint32_t slot_address = 0u;
    PackageArchiveFileSlot* slot = nullptr;
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        const auto address = archive.ReadWord(
            PackageArchiveState::kSlotArray + i * 4u);
        auto* candidate = runtime.Slot(address);
        if (candidate != nullptr && candidate->active == 0u) {
            slot_address = address;
            slot = candidate;
            break;
        }
    }
    if (slot == nullptr) {
        runtime.ReportError(PackageArchiveEntryError::TooManyOpenFiles);
        return 0u;
    }

    slot->active = 1u;
    slot->mapped_base = 0u;
    if (mapped) {
        slot->mapped_base = runtime.MapView(
            archive.ReadWord(1048752u), 4u, aligned, mapping_bytes);
        if (slot->mapped_base == 0u) {
            runtime.ReportError(PackageArchiveEntryError::CannotMapFile);
            std::string archive_path;
            for (std::uint32_t i = 0u; i < 0x104u; ++i) {
                const char value = static_cast<char>(archive.ReadByte(1048756u + i));
                if (value == '\0') break;
                archive_path.push_back(value);
            }
            runtime.ReportMapFailure(archive_path.c_str(), path,
                                     aligned, mapping_bytes);
            slot->active = 0u;
            return 0u;
        }
    }

    slot->file_id = archive.ReadWord(record);
    slot->logical_bytes = archive.ReadWord(record + 8u);
    slot->index = index;
    slot->cursor = slot->mapped_base + delta;
    slot->mapping_delta = delta;
    slot->needs_decode =
        static_cast<std::uint8_t>(
            (archive.ReadByte(record + 6u) |
             (std::uint16_t(archive.ReadByte(record + 7u)) << 8u)) != 1u);
    slot->decoded_bytes = archive.ReadWord(record + 20u);
    slot->reserved40 = 0u;
    slot->record_offset = record;

    if (slot->needs_decode != 0u && slot->decoded_bytes != 0u) {
        if (!mapped) {
            slot->active = 0u;
            return 0u;
        }
        runtime.EnsureDecoderAllocator();
        slot->data = runtime.AllocateDecoded(slot->decoded_bytes);
        if (slot->data == 0u) {
            runtime.ReportError(PackageArchiveEntryError::CannotAllocateDecoded);
            (void)runtime.UnmapView(slot->mapped_base);
            slot->active = 0u;
            return 0u;
        }
        const std::uint16_t type = std::uint16_t(
            archive.ReadByte(record + 6u) |
            (std::uint16_t(archive.ReadByte(record + 7u)) << 8u));
        if (!runtime.Decode(type, archive.ReadWord(record + 16u),
                            slot->decoded_bytes, slot->cursor, slot->data)) {
            runtime.ReportError(PackageArchiveEntryError::CannotDecompress);
            (void)runtime.UnmapView(slot->mapped_base);
            runtime.EnsureDecoderAllocator();
            runtime.FreeDecoded(slot->data, slot->decoded_bytes);
            slot->active = 0u;
            return 0u;
        }
    } else {
        slot->data = slot->cursor;
    }
    if (!mapped)
        runtime.SetFilePointer(archive.ReadWord(1048748u), offset);
    archive.WriteWord(1049016u, archive.ReadWord(1049016u) + 1u);
    return slot_address;
}

bool PackageArchiveFileEntry::Close(
    PackageArchiveState& archive, const std::uint32_t slot_address,
    IPackageArchiveEntryRuntime& runtime) {
    auto* slot = runtime.Slot(slot_address);
    if (slot == nullptr) return false;
    if (archive.ReadWord(4u) == 2u) {
        if (slot->mapped_base == 0u) {
            runtime.ReportError(PackageArchiveEntryError::InvalidMappedAddress);
            return false;
        }
        if (!runtime.UnmapView(slot->mapped_base)) {
            runtime.ReportError(PackageArchiveEntryError::CannotUnmapFile);
            slot->active = 0u;
            return false;
        }
        if (slot->needs_decode != 0u && slot->decoded_bytes != 0u) {
            runtime.EnsureDecoderAllocator();
            runtime.FreeDecoded(slot->data, slot->decoded_bytes);
        }
    }
    slot->active = 0u;
    archive.WriteWord(1049016u, archive.ReadWord(1049016u) - 1u);
    return true;
}

std::uint32_t PackageArchiveFileEntry::ReadData(
    const PackageArchiveState& archive, PackageArchiveFileSlot& slot,
    void* destination, const std::uint32_t bytes,
    IPackageArchiveReadRuntime& runtime) {
    if (archive.ReadWord(4u) == 2u)
        runtime.CopyFromAddress(destination, slot.data + slot.reserved40, bytes);
    else
        runtime.ReadFile(archive.ReadWord(1048748u), destination, bytes);
    slot.reserved40 += bytes;
    return (slot.reserved40 & ~0xffu) | 1u;
}

std::uint32_t PackageArchiveFileEntry::Seek(
    const PackageArchiveState& archive, PackageArchiveFileSlot& slot,
    const std::int32_t offset, const std::uint32_t origin,
    IPackageArchiveEntryRuntime& runtime) {
    const auto displacement = static_cast<std::uint32_t>(offset);
    switch (origin) {
        case 0u: slot.reserved40 = displacement; break;
        case 1u: slot.reserved40 += displacement; break;
        case 2u: slot.reserved40 = slot.decoded_bytes - displacement; break;
        default: return 0xffffffffu;
    }
    if (archive.ReadWord(4u) != 2u)
        runtime.SetFilePointer(
            archive.ReadWord(1048748u),
            archive.ReadWord(slot.record_offset + 12u) + slot.reserved40);
    return slot.reserved40;
}

void PackageArchiveFileEntry::LoadToBuffer(
    const PackageArchiveState& archive, const PackageArchiveFileSlot* slot,
    PackageFileBufferState* buffer, IPackageFileBufferRuntime& runtime) {
    if (slot == nullptr || buffer == nullptr || slot->index == 0xffffffffu)
        return;
    if (slot->index >= 0x100000u / 32u)
        throw std::out_of_range("package file index exceeds table");
    const std::uint32_t record = 136u + slot->index * 32u;
    (void)PackageFileBuffer::LoadFromHandle(
        *buffer, archive.ReadWord(1048748u), slot->decoded_bytes,
        archive.ReadWord(record + 28u) != 0u, runtime);
}

}  // namespace pal4
