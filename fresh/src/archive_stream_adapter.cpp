#include "pal4/archive_stream_adapter.h"

namespace pal4 {

ArchiveStreamAdapterState& ArchiveStreamAdapter::Construct(
    ArchiveStreamAdapterState& state, std::uint32_t stream,
    std::uint8_t mode) {
    state.stream = stream;
    state.mode = mode;
    return state;
}

std::uint32_t ArchiveStreamAdapter::Write(
    const ArchiveStreamAdapterState& state, const void* buffer,
    std::uint32_t bytes) {
    return runtime_.InvokeVirtual(state.stream, 16u,
                                  reinterpret_cast<std::uintptr_t>(buffer),
                                  bytes);
}

std::uint32_t ArchiveStreamAdapter::Read(
    const ArchiveStreamAdapterState& state, void* buffer,
    std::uint32_t bytes) {
    return runtime_.InvokeVirtual(state.stream, 12u,
                                  reinterpret_cast<std::uintptr_t>(buffer),
                                  bytes);
}

}  // namespace pal4
