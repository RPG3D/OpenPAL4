#include "pal4/archive_file_stream.h"

namespace pal4 {

ArchiveFileStreamState& ArchiveFileStream::Construct(
    ArchiveFileStreamState& state, std::uint32_t file) {
    state.vtable = kVtable;
    state.file = file;
    static_cast<void>(runtime_.Seek(file, 0, 0));
    return state;
}

std::int32_t ArchiveFileStream::Size(const ArchiveFileStreamState& state) {
    const auto previous = runtime_.Tell(state.file);
    static_cast<void>(runtime_.Seek(state.file, 0, 2));
    const auto size = runtime_.Tell(state.file);
    static_cast<void>(runtime_.Seek(state.file, previous, 0));
    return size;
}

std::int32_t ArchiveFileStream::Position(
    const ArchiveFileStreamState& state) {
    return runtime_.Tell(state.file);
}

std::int32_t ArchiveFileStream::SetPosition(
    const ArchiveFileStreamState& state, std::int32_t offset) {
    return runtime_.Seek(state.file, offset, 0);
}

bool ArchiveFileStream::ReadElement(const ArchiveFileStreamState& state,
                                    void* buffer, std::uint32_t size) {
    return runtime_.Read(buffer, size, 1u, state.file) != 0u;
}

bool ArchiveFileStream::WriteElement(const ArchiveFileStreamState& state,
                                     const void* buffer, std::uint32_t size) {
    return runtime_.Write(buffer, size, 1u, state.file) != 0u;
}

}  // namespace pal4
