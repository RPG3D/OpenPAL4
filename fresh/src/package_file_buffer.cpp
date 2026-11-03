#include "pal4/package_file_buffer.h"

namespace pal4 {

PackageFileBufferState& PackageFileBuffer::Initialize(
    PackageFileBufferState& state) noexcept {
    state.loaded = 0u;
    state.data = 0u;
    state.auxiliary = 0u;
    state.bytes = 0u;
    return state;
}

std::uint32_t PackageFileBuffer::ReleaseThunk(
    PackageFileBufferState& state, IPackageFileBufferRuntime& runtime) {
    return Release(state, runtime);
}

std::uint32_t PackageFileBuffer::Release(
    PackageFileBufferState& state, IPackageFileBufferRuntime& runtime) {
    std::uint32_t result = state.data;
    if (state.data != 0u) {
        result = runtime.Free(state.data);
        state.data = 0u;
    }
    state.auxiliary = 0u;
    state.bytes = 0u;
    state.loaded = 0u;
    return result;
}

std::uint32_t PackageFileBuffer::LoadFromHandle(
    PackageFileBufferState& state, const std::uint32_t handle,
    const std::uint32_t bytes, const bool encrypted,
    IPackageFileBufferRuntime& runtime) {
    (void)Release(state, runtime);
    state.bytes = bytes;
    state.data = runtime.Allocate(bytes);
    std::uint32_t result = runtime.Read(handle, state.data, bytes);
    if (state.data != 0u) {
        // The original clears AL with the false encryption flag before exit.
        result = (result & ~0xffu) | static_cast<std::uint32_t>(encrypted);
        if (encrypted)
            result = runtime.Decrypt(state.data, bytes >> 2u, kDecryptionKey);
    }
    state.loaded = 1u;
    return result;
}

}  // namespace pal4
