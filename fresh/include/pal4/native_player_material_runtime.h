#pragma once

#include "pal4/base_object_material_texture.h"
#include "pal4/native_package_archive.h"
#include "pal4/rw_texture_dictionary.h"
#include "pal4/legacy_crc32.h"
#include "pal4/script_player_material.h"

#include <filesystem>
#include <unordered_map>

namespace pal4 {
class NativePkFileManagerRuntime;
class NativePlayerManagerSession;

enum class NativeMaterialResourceFamily { Actor, Weapon, Object };
// Owns cached textures through the native CPK/RenderWare boundary.
// Materials retain their own SDK references. Destroy before RenderWare closes.
class NativePlayerMaterialRuntime final : public IBaseObjectMaterialRuntime {
public:
    NativePlayerMaterialRuntime(std::filesystem::path root, NativePkFileManagerRuntime& packages,
        NativeMaterialResourceFamily family = NativeMaterialResourceFamily::Actor);
    ~NativePlayerMaterialRuntime() override;
    NativePlayerMaterialRuntime(const NativePlayerMaterialRuntime&) = delete;
    NativePlayerMaterialRuntime& operator=(const NativePlayerMaterialRuntime&) = delete;
    std::string CreatePath(std::uint32_t source) override;
    void AddAndProcessScripts() override;
    void CleanupScripts() override;
    std::uint32_t TextureDictionary() override;
    std::uint32_t ResolveTexture(std::uint32_t dictionary, std::string_view path) override;
    std::uint32_t ResolveStoredTexture(std::uint32_t dictionary, std::uint32_t path) override;
    void SetMaterialTexture(std::uint32_t material, std::uint32_t texture) override;
    void ClearCache() noexcept;
    [[nodiscard]] std::size_t CachedTextureCount() const noexcept { return textures_.size(); }
private:
    std::filesystem::path root_;
    NativePkFileManagerRuntime& packages_;
    NativeMaterialResourceFamily family_;
    NativePackageArchive archive_;
    std::unordered_map<std::string, std::uint32_t> textures_;
    RwTextureDictionaryState dictionary_{};
    LegacyCrc32 crc_;
    bool scope_active_{};
    bool archive_open_{};
};

// Borrows persistent Player slots and the existing CPK/RenderWare texture cache.
class NativeScriptPlayerMaterialRuntime final : public IScriptPlayerTextureRuntime {
public:
    NativeScriptPlayerMaterialRuntime(NativePlayerManagerSession& players,
                                     IBaseObjectMaterialRuntime& textures)
        : players_(players), textures_(textures) {}
    std::uint32_t SetFace(std::int32_t player, std::string_view texture) override;
    std::uint32_t ApplyFace(std::int32_t player) override;
    std::uint32_t SetLingshaLegs(std::string_view texture) override;
    std::uint32_t ApplyLingshaLegs() override;
private:
    NativePlayerManagerSession& players_;
    IBaseObjectMaterialRuntime& textures_;
};
}  // namespace pal4
