#include "pal4/cegui_package_file_stream.h"

namespace pal4 {

std::uint32_t CeguiPackageFileStreamState::ReadDword(
    const std::uint32_t offset) const noexcept {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(bytes_[offset + index])
                 << (8u * index);
    }
    return value;
}
void CeguiPackageFileStreamState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_[offset + index] =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

std::uint32_t CeguiPackageFileRuntimeStub::Free(
    std::uint32_t /*address*/) { return 0u; }
std::uint32_t CeguiPackageFileRuntimeStub::BaseVtable() { return 0u; }
void CeguiPackageFileRuntimeStub::LoadUiScriptPath(
    std::uint32_t /*resource_address*/, char* output,
    const std::uint32_t capacity) {
    if (capacity != 0u) {
        output[0] = '\0';
    }
}
std::uint32_t CeguiPackageFileRuntimeStub::InitializeManager() { return 0u; }
std::uint32_t CeguiPackageFileRuntimeStub::OpenFile(
    std::uint32_t /*manager*/, const char* /*path*/) { return 0u; }
std::int32_t CeguiPackageFileRuntimeStub::SeekFile(
    std::uint32_t /*manager*/, std::uint32_t /*handle*/,
    std::int32_t /*offset*/, std::uint32_t /*origin*/) { return -1; }
void CeguiPackageFileRuntimeStub::CloseFile(
    std::uint32_t /*manager*/, std::uint32_t /*handle*/) {}
bool CeguiPackageFileRuntimeStub::ReadFile(
    std::uint32_t /*manager*/, std::uint32_t /*buffer*/,
    std::uint32_t /*bytes*/, std::uint32_t /*handle*/) { return false; }
std::int32_t CeguiPackageFileRuntimeStub::ReadPosition(
    std::uint32_t /*handle*/) { return -1; }

CeguiPackageFileStreamState& CeguiPackageFileStream::Construct(
    CeguiPackageFileStreamState& state) noexcept {
    state.WriteDword(0u, CeguiPackageFileStreamState::kVtable);
    state.WriteDword(4u, 0u);
    return state;
}

CeguiPackageFileStreamState& CeguiPackageFileStream::Destroy(
    CeguiPackageFileStreamState& state, const std::uint8_t flags,
    const std::uint32_t object_address,
    ICeguiPackageFileRuntime& runtime) {
    static_cast<void>(Deinitialize(state, runtime));
    if ((flags & 1u) != 0u) {
        static_cast<void>(runtime.Free(object_address));
    }
    return state;
}

std::uint32_t CeguiPackageFileStream::Deinitialize(
    CeguiPackageFileStreamState& state,
    ICeguiPackageFileRuntime& runtime) {
    state.WriteDword(0u, CeguiPackageFileStreamState::kVtable);
    static_cast<void>(Close(state, runtime));
    const auto base_vtable = runtime.BaseVtable();
    state.WriteDword(0u, base_vtable);
    return base_vtable;
}

bool CeguiPackageFileStream::Open(
    CeguiPackageFileStreamState& state,
    const std::uint32_t resource_address, const std::uint32_t unused,
    ICeguiPackageFileRuntime& runtime) {
    static_cast<void>(unused);
    static_cast<void>(Close(state, runtime));
    std::array<char, 256> path{};
    runtime.LoadUiScriptPath(resource_address, path.data(),
                             static_cast<std::uint32_t>(path.size()));
    const auto first_manager = kManagerBase + runtime.InitializeManager();
    const auto handle = runtime.OpenFile(first_manager, path.data());
    state.WriteDword(4u, handle);
    const auto second_manager = kManagerBase + runtime.InitializeManager();
    static_cast<void>(runtime.SeekFile(second_manager, handle, 0, 0u));
    return handle != 0u;
}

std::int32_t CeguiPackageFileStream::Close(
    CeguiPackageFileStreamState& state,
    ICeguiPackageFileRuntime& runtime) {
    const auto handle = state.ReadDword(4u);
    if (handle != 0u) {
        const auto manager = kManagerBase + runtime.InitializeManager();
        runtime.CloseFile(manager, handle);
        state.WriteDword(4u, 0u);
    }
    return 0;
}

std::uint32_t CeguiPackageFileStream::Read(
    const CeguiPackageFileStreamState& state,
    const std::uint32_t buffer, const std::uint32_t size,
    const std::uint32_t count,
    ICeguiPackageFileRuntime& runtime) {
    const auto handle = state.ReadDword(4u);
    if (handle == 0u) {
        return 0u;
    }
    const auto manager = kManagerBase + runtime.InitializeManager();
    const auto bytes = size * count;
    return runtime.ReadFile(manager, buffer, bytes, handle) ? bytes : 0u;
}

std::int32_t CeguiPackageFileStream::Seek(
    const CeguiPackageFileStreamState& state,
    const std::int32_t offset, const std::uint32_t origin,
    ICeguiPackageFileRuntime& runtime) {
    const auto handle = state.ReadDword(4u);
    if (handle == 0u || origin > 2u) {
        return -1;
    }
    const auto manager = kManagerBase + runtime.InitializeManager();
    return runtime.SeekFile(manager, handle, offset, origin);
}

std::int32_t CeguiPackageFileStream::Position(
    const CeguiPackageFileStreamState& state,
    ICeguiPackageFileRuntime& runtime) {
    const auto handle = state.ReadDword(4u);
    if (handle == 0u) {
        return -1;
    }
    static_cast<void>(runtime.InitializeManager());
    return runtime.ReadPosition(handle);
}

}  // namespace pal4
