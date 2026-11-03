#include "pal4/archive_stream_proxy.h"

namespace pal4 {

ArchiveStreamProxyState& ArchiveStreamProxy::Construct(
    ArchiveStreamProxyState& state, std::uint32_t adapter) {
    state.vtable = kVtable;
    state.adapter = adapter;
    return state;
}

std::uint32_t ArchiveStreamProxy::Destroy(
    ArchiveStreamProxyState& state, std::uint32_t object,
    std::uint32_t flags) {
    state.vtable = kBaseVtable;
    if ((flags & 1u) != 0u) runtime_.FreeMemory(object);
    return object;
}

std::uint32_t ArchiveStreamProxy::Stream(
    const ArchiveStreamProxyState& state) {
    return runtime_.ReadWord(state.adapter);
}

std::uint32_t ArchiveStreamProxy::CallSlot0(
    const ArchiveStreamProxyState& state, std::uintptr_t arg0,
    std::uint32_t arg1) {
    return runtime_.InvokeVirtual(Stream(state), 0u, arg0, arg1);
}

std::uint32_t ArchiveStreamProxy::CallSlot4(
    const ArchiveStreamProxyState& state, std::uintptr_t arg0,
    std::uint32_t arg1) {
    return runtime_.InvokeVirtual(Stream(state), 4u, arg0, arg1);
}

std::uint32_t ArchiveStreamProxy::CallSlot8(
    const ArchiveStreamProxyState& state, std::uintptr_t arg0,
    std::uint32_t arg1) {
    return runtime_.InvokeVirtual(Stream(state), 8u, arg0, arg1);
}

std::uint32_t ArchiveStreamProxy::Read(
    const ArchiveStreamProxyState& state, void* buffer,
    std::uint32_t bytes) {
    ArchiveStreamAdapterState adapter_state{Stream(state)};
    return adapter_.Read(adapter_state, buffer, bytes);
}

std::uint32_t ArchiveStreamProxy::Write(
    const ArchiveStreamProxyState& state, const void* buffer,
    std::uint32_t bytes) {
    ArchiveStreamAdapterState adapter_state{Stream(state)};
    return adapter_.Write(adapter_state, buffer, bytes);
}

}  // namespace pal4
