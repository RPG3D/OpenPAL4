#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

struct RpClump;

namespace pal4 {

// Owns one resource Clump from PALObject, PALWeapon, PALWorld, UI or Effect CPK.
class NativeGobClumpAsset final {
public:
    NativeGobClumpAsset() = default;
    ~NativeGobClumpAsset();
    NativeGobClumpAsset(const NativeGobClumpAsset&) = delete;
    NativeGobClumpAsset& operator=(const NativeGobClumpAsset&) = delete;

    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view resource_dir,
                            std::string_view model);
    void Unload() noexcept;
    [[nodiscard]] RpClump* Clump() const noexcept { return clump_; }
    [[nodiscard]] const std::filesystem::path& PackagePath() const noexcept {
        return package_path_;
    }
    [[nodiscard]] const std::string& MemberDirectory() const noexcept {
        return member_directory_;
    }
    [[nodiscard]] const std::string& ResourceDirectory() const noexcept {
        return resource_directory_;
    }
    [[nodiscard]] std::uint32_t AtomicCount() const noexcept {
        return atomics_;
    }
    [[nodiscard]] std::uint32_t TextureRequests() const noexcept {
        return texture_requests_;
    }
    [[nodiscard]] std::uint32_t MissingTextures() const noexcept {
        return missing_textures_;
    }
    [[nodiscard]] std::uint32_t TextureDecodeFailures() const noexcept {
        return texture_decode_failures_;
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    RpClump* clump_ = nullptr;
    std::filesystem::path package_path_;
    std::string member_directory_;
    std::string resource_directory_;
    std::uint32_t atomics_ = 0u;
    std::uint32_t textures_ = 0u;
    std::uint32_t texture_requests_ = 0u;
    std::uint32_t missing_textures_ = 0u;
    std::uint32_t texture_decode_failures_ = 0u;
    std::string last_error_;
};

}  // namespace pal4
