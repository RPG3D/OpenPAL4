#pragma once

#include "pal4/rw_dict_schema.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

struct RpClump;
struct RpLight;
struct RpMaterial;
struct RpWorld;
struct RwTexDictionary;

namespace pal4 {

struct GameWorldObjectState;
class NativePackageArchive;

// Owns a world decoded from an original PAL4 CPK archive.
class NativeWorldAsset final {
public:
    NativeWorldAsset() = default;
    ~NativeWorldAsset();
    NativeWorldAsset(const NativeWorldAsset&) = delete;
    NativeWorldAsset& operator=(const NativeWorldAsset&) = delete;

    [[nodiscard]] bool LoadUi(const std::filesystem::path& game_root,
                              std::string_view name);
    [[nodiscard]] bool LoadArenaSection(
        const std::filesystem::path& game_root,
        std::string_view arena, std::string_view section);
    // Arena loads retain detached auxiliary clumps; the scene stage binds
    // trans/clip/clipNA. LoadUi performs its separate trans-only association.
    [[nodiscard]] bool LoadArenaSection(NativePackageArchive& archive,
                                        std::string_view section);
    [[nodiscard]] bool AddAmbientLight(float red, float green, float blue,
                                       float alpha);
    void Unload() noexcept;
    // Move schema storage into the actual GameWorld image. The image must
    // outlive this asset's loaded resources; Unload clears it before detaching.
    [[nodiscard]] bool BindAnimationStorage(GameWorldObjectState& state);
    [[nodiscard]] const RwDictSchema& AnimationSchema(std::size_t slot) const;
    [[nodiscard]] RpWorld* World() const noexcept { return world_; }
    [[nodiscard]] RpLight* Light() const noexcept { return light_; }
    [[nodiscard]] std::uint32_t SectorCount() const noexcept { return sectors_; }
    [[nodiscard]] std::size_t ClumpCount() const noexcept;
    // Slot order matches the five optional GameWorld clumps.
    [[nodiscard]] RpClump* ClumpAt(std::size_t slot) const noexcept {
        return slot < clump_slots_.size() ? clump_slots_[slot] : nullptr;
    }
    [[nodiscard]] std::uint32_t TextureCount() const noexcept {
        return textures_;
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
    [[nodiscard]] const std::string& FirstMissingTexture() const noexcept {
        return first_missing_texture_;
    }
    [[nodiscard]] std::size_t DictionaryCount() const noexcept;
    [[nodiscard]] std::uint32_t AnimatedMaterialCount() const noexcept {
        return animated_materials_;
    }
    [[nodiscard]] std::optional<float> FirstAnimationTime() const noexcept;
    [[nodiscard]] bool UpdateAnimation(float elapsed);
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    friend class NativeGameWorldCleanupRuntime;
    [[nodiscard]] bool LoadPackage(const std::filesystem::path& package,
                                   const std::string& member_prefix,
                                   const std::string& texture_prefix);
    [[nodiscard]] bool LoadArchive(NativePackageArchive& archive,
                                   const std::string& member_prefix,
                                   const std::string& texture_prefix);
    RpWorld* world_ = nullptr;
    RpLight* light_ = nullptr;
    std::array<RpClump*, 5> clump_slots_{};
    // 662180/662000 select distinct dictionaries for the BSP and each DFF.
    // Keep texture ownership separate from the UV-animation dictionaries.
    std::array<RwTexDictionary*, 6> texture_dictionaries_{};
    std::array<RwDictSchema, 6> animation_schemas_{};
    GameWorldObjectState* animation_storage_ = nullptr;
    [[nodiscard]] RwDictSchema& MutableAnimationSchema(std::size_t slot);
    std::uint32_t sectors_ = 0;
    std::uint32_t textures_ = 0;
    std::uint32_t texture_requests_ = 0;
    std::uint32_t missing_textures_ = 0;
    std::uint32_t texture_decode_failures_ = 0;
    std::uint32_t animated_materials_ = 0;
    std::string first_missing_texture_;
    std::string last_error_;
};

}  // namespace pal4
