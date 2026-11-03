#include "pal4/cegui_resource_loader.h"

namespace pal4 {

std::uint32_t CeguiRawDataContainer::ReadDword(
    const std::uint32_t offset) const noexcept {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(bytes_[offset + index])
                 << (8u * index);
    }
    return value;
}
void CeguiRawDataContainer::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_[offset + index] =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

CeguiInvalidRequest::CeguiInvalidRequest(const Reason reason)
    : std::runtime_error(reason == Reason::EmptyName
                             ? "CEGUI resource name is empty"
                             : "CEGUI resource file does not exist"),
      reason_(reason) {}
CeguiInvalidRequest::Reason CeguiInvalidRequest::GetReason() const noexcept {
    return reason_;
}

std::uint32_t CeguiResourceLoaderRuntimeStub::StringStorage(
    std::uint32_t /*string_address*/) { return 0u; }
bool CeguiResourceLoaderRuntimeStub::EqualsEmptyString(
    std::uint32_t /*string_address*/) { return true; }
bool CeguiResourceLoaderRuntimeStub::UsePackageFiles() { return false; }
std::uint32_t CeguiResourceLoaderRuntimeStub::BuildUtf8(
    std::uint32_t /*string_address*/) { return 0u; }
void CeguiResourceLoaderRuntimeStub::LoadUiScriptPath(
    std::uint32_t /*utf8_address*/, char* output,
    const std::uint32_t capacity) {
    if (capacity != 0u) {
        output[0] = '\0';
    }
}
std::uint32_t CeguiResourceLoaderRuntimeStub::InitializeManager() {
    return 0u;
}
std::uint32_t CeguiResourceLoaderRuntimeStub::PackageOpen(
    std::uint32_t /*manager*/, const char* /*path*/) { return 0u; }
std::uint32_t CeguiResourceLoaderRuntimeStub::PackageSize(
    std::uint32_t /*handle*/) { return 0u; }
std::uint32_t CeguiResourceLoaderRuntimeStub::CreateBuffer(
    std::uint32_t /*provider*/, std::uint32_t /*size*/) { return 0u; }
bool CeguiResourceLoaderRuntimeStub::PackageRead(
    std::uint32_t /*manager*/, std::uint32_t /*buffer*/,
    std::uint32_t /*bytes*/, std::uint32_t /*handle*/) { return false; }
void CeguiResourceLoaderRuntimeStub::Free(
    std::uint32_t /*address*/) {}
void CeguiResourceLoaderRuntimeStub::PackageClose(
    std::uint32_t /*manager*/, std::uint32_t /*handle*/) {}
bool CeguiResourceLoaderRuntimeStub::LocalOpen(
    std::uint32_t /*utf8_address*/) { return false; }
std::uint32_t CeguiResourceLoaderRuntimeStub::LocalSize() { return 0u; }
void CeguiResourceLoaderRuntimeStub::LocalRead(
    std::uint32_t /*buffer*/, std::uint32_t /*size*/) {}
bool CeguiResourceLoaderRuntimeStub::LocalClose() { return false; }

void CeguiResourceLoader::Load(
    const std::uint32_t provider, const std::uint32_t name_address,
    CeguiRawDataContainer& output, const std::uint32_t unused,
    ICeguiResourceLoaderRuntime& runtime) {
    static_cast<void>(unused);
    if (runtime.StringStorage(name_address) == 0u ||
        runtime.EqualsEmptyString(name_address)) {
        throw CeguiInvalidRequest(CeguiInvalidRequest::Reason::EmptyName);
    }
    if (runtime.UsePackageFiles()) {
        const auto utf8 = runtime.BuildUtf8(name_address);
        std::array<char, 256> path{};
        runtime.LoadUiScriptPath(utf8, path.data(),
                                 static_cast<std::uint32_t>(path.size()));
        const auto open_manager = kManagerBase + runtime.InitializeManager();
        const auto handle = runtime.PackageOpen(open_manager, path.data());
        if (handle == 0u) {
            throw CeguiInvalidRequest(CeguiInvalidRequest::Reason::MissingFile);
        }
        std::uint32_t buffer = 0u;
        try {
            static_cast<void>(runtime.InitializeManager());
            const auto size = runtime.PackageSize(handle);
            buffer = runtime.CreateBuffer(provider, size);
            const auto read_manager = kManagerBase + runtime.InitializeManager();
            if (runtime.PackageRead(read_manager, buffer, size, handle)) {
                output.WriteDword(4u, buffer);
                output.WriteDword(8u, size);
                buffer = 0u;
            } else {
                runtime.Free(buffer);
                buffer = 0u;
            }
        } catch (...) {
            if (buffer != 0u) {
                runtime.Free(buffer);
            }
            const auto close_manager = kManagerBase + runtime.InitializeManager();
            runtime.PackageClose(close_manager, handle);
            throw;
        }
        const auto close_manager = kManagerBase + runtime.InitializeManager();
        runtime.PackageClose(close_manager, handle);
        return;
    }

    const auto utf8 = runtime.BuildUtf8(name_address);
    if (!runtime.LocalOpen(utf8)) {
        throw CeguiInvalidRequest(CeguiInvalidRequest::Reason::MissingFile);
    }
    const auto size = runtime.LocalSize();
    const auto buffer = runtime.CreateBuffer(provider, size);
    try {
        runtime.LocalRead(buffer, size);
        static_cast<void>(runtime.LocalClose());
    } catch (...) {
        runtime.Free(buffer);
        static_cast<void>(runtime.LocalClose());
        throw;
    }
    output.WriteDword(8u, size);
    output.WriteDword(4u, buffer);
}

}  // namespace pal4
