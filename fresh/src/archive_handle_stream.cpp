#include "pal4/archive_handle_stream.h"

namespace pal4 {

ArchiveFileStreamState& ArchiveHandleStream::Construct(
    ArchiveFileStreamState& state, std::uint32_t handle) {
    state.vtable = kVtable;
    state.file = handle;
    static_cast<void>(runtime_.Seek(handle, 0, 0));
    return state;
}

std::int32_t ArchiveHandleStream::Size(
    const ArchiveFileStreamState& state) {
    const auto previous = runtime_.Position(state.file);
    static_cast<void>(runtime_.Seek(state.file, 0, 2));
    const auto size = runtime_.Position(state.file);
    static_cast<void>(runtime_.Seek(state.file, previous, 0));
    return size;
}

std::int32_t ArchiveHandleStream::SetPosition(
    const ArchiveFileStreamState& state, std::int32_t offset) {
    return runtime_.Seek(state.file, offset, 0);
}

bool ArchiveHandleStream::ReadElement(
    const ArchiveFileStreamState& state, void* buffer, std::uint32_t size) {
    return runtime_.Read(buffer, size, 1u, state.file) != 0u;
}

bool ArchiveHandleStream::WriteElement(
    const ArchiveFileStreamState& state, const void* buffer,
    std::uint32_t size) {
    return runtime_.Write(buffer, size, 1u, state.file) != 0u;
}

}  // namespace pal4
