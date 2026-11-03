#pragma once

#include "pal4/native_player_weapon_runtime.h"
#include "pal4/native_player_material_runtime.h"
#include <array>
#include <filesystem>
#include <memory>

namespace pal4 {
class NativePkFileManagerRuntime;

class INativePlayerNamedWeaponHost : public INativePlayerWeaponSetupHost {
public:
    virtual BaseObjectEffectHandle LoadXmlEffect(std::uint32_t system,
        std::uint32_t name, std::uint32_t clump) = 0;
};

// Owns up to four players' two named weapon resources. Player state, world,
// package/transform/effect services must outlive this runtime.
class NativePlayerNamedWeaponRuntime final : public NativePlayerWeaponRuntime {
public:
    NativePlayerNamedWeaponRuntime(std::filesystem::path root, RpWorld& world,
        std::uint32_t game, NativePkFileManagerRuntime& packages,
        INativePlayerNamedWeaponHost& host, NativePlayerTransformRuntime& transforms);
    ~NativePlayerNamedWeaponRuntime() override;
    void AddAndProcessScripts() override;
    void CleanupScripts() override;
    std::string CreatePath(std::uint32_t directory) override;
    void CleanupPath(std::string&) override;
    bool LoadClump(BaseObjectState&, std::uint32_t resource, std::string_view label,
        std::string_view path, std::string_view model, std::uint32_t context,
        std::uint32_t option, std::uint32_t extra, std::uint32_t effects) override;
    void CleanupClump(BaseObjectState&, std::uint32_t resource, std::uint32_t context) override;
    void HideHookAtomics(std::uint32_t clump) override;
    std::int32_t CompareRoleToLinsha(std::uint32_t name) override;
    std::uint32_t TextureDictionary() override;
    std::uint32_t ResolveTexturePath(std::uint32_t dictionary, std::string_view path) override;
    void LoadTexture(BaseObjectState&, std::uint32_t resource,
        std::string_view name, std::uint32_t texture) override;
    BaseObjectEffectHandle LoadXmlEffect(std::uint32_t system, std::uint32_t name,
        std::uint32_t clump) override;
    [[nodiscard]] std::size_t OwnedCount() const noexcept;
private:
    struct Entry;
    static void Unbind(Entry&) noexcept;
    std::filesystem::path root_;
    NativePkFileManagerRuntime& packages_;
    INativePlayerNamedWeaponHost& host_;
    NativePlayerTransformRuntime& transforms_;
    NativePlayerMaterialRuntime textures_;
    std::array<std::unique_ptr<Entry>,8u> entries_;
    bool scope_active_{};
};
}  // namespace pal4
