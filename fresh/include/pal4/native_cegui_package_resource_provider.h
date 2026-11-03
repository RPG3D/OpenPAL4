#pragma once

#include "pal4/native_package_archive.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

namespace pal4 {

class NativeCeguiPackageResourceProvider final {
public:
    NativeCeguiPackageResourceProvider() = default;
    ~NativeCeguiPackageResourceProvider();
    NativeCeguiPackageResourceProvider(
        const NativeCeguiPackageResourceProvider&) = delete;
    NativeCeguiPackageResourceProvider& operator=(
        const NativeCeguiPackageResourceProvider&) = delete;

    [[nodiscard]] bool Bind(HMODULE cegui, void* system,
                            const char* game_root);
    void Unbind() noexcept;
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }
    [[nodiscard]] std::uint32_t LoadCount() const noexcept {
        return load_count_;
    }
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadMember(
        const char* path) const;
    // IDA66F780/66F980 temporarily restore native-file callbacks for saves.
    // Both XML/resource requests and texture requests must use the same mode.
    // Empty root inherits the active filesystem scope, or the asset root when
    // entering from package mode. A root override never changes process cwd.
    void PushFilesystemAccess(const std::filesystem::path& root = {});
    void PopFilesystemAccess();
    // IDA670060/66F580: temporarily select packed resources, restoring the
    // previous callback mode even when nested inside filesystem access.
    void PushPackageAccess();
    void PopPackageAccess();
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadResource(
        const char* path) const;
    [[nodiscard]] std::string TextureCacheKey(const char* path) const;

private:
    using CreateBuffer = void* (__thiscall*)(void*, unsigned int);
    using StringCStr = const char* (__thiscall*)(const void*);

    static void __fastcall Load(void* self, void* unused,
                                const void* filename, void* output,
                                const void* group) noexcept;
    void LoadResource(void* self, const void* filename,
                      void* output) noexcept;

    static NativeCeguiPackageResourceProvider* active_;
    std::unique_ptr<NativePackageArchive> archive_;
    void* provider_ = nullptr;
    std::uintptr_t* original_vtable_ = nullptr;
    std::array<std::uintptr_t, 3> vtable_{};
    CreateBuffer create_buffer_ = nullptr;
    StringCStr string_c_str_ = nullptr;
    std::string last_error_;
    std::uint32_t load_count_ = 0;
    std::filesystem::path game_root_;
    std::vector<std::optional<std::filesystem::path>> access_modes_;
};

}  // namespace pal4
