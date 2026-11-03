#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

struct RpClump;
struct RwTexDictionary;
struct RpLight;

namespace pal4 {

// Owns one actor's main and shadow clumps decoded from the original CPK.
class NativeActorClumpAsset final {
public:
    ~NativeActorClumpAsset();
    NativeActorClumpAsset(const NativeActorClumpAsset&) = delete;
    NativeActorClumpAsset& operator=(const NativeActorClumpAsset&) = delete;
    NativeActorClumpAsset() = default;

    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view model);
    void Unload() noexcept;
    [[nodiscard]] RpClump* Main() const noexcept { return main_; }
    [[nodiscard]] RpClump* Shadow() const noexcept { return shadow_; }
    [[nodiscard]] RpLight* ShadowLight() const noexcept { return shadow_light_; }
    [[nodiscard]] std::uint32_t AtomicCount() const noexcept {
        return atomics_;
    }
    [[nodiscard]] std::uint32_t MaterialCount() const noexcept {
        return materials_;
    }
    [[nodiscard]] std::uint32_t TexturedMaterialCount() const noexcept {
        return textured_materials_;
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
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    RpClump* main_ = nullptr;
    RpClump* shadow_ = nullptr;
    RpLight* shadow_light_ = nullptr;
    RwTexDictionary* main_dictionary_ = nullptr;
    RwTexDictionary* shadow_dictionary_ = nullptr;
    std::uint32_t atomics_ = 0u;
    std::uint32_t materials_ = 0u;
    std::uint32_t textured_materials_ = 0u;
    std::uint32_t textures_ = 0u;
    std::uint32_t texture_requests_ = 0u;
    std::uint32_t missing_textures_ = 0u;
    std::uint32_t texture_decode_failures_ = 0u;
    std::string first_missing_texture_;
    std::string last_error_;
};

}  // namespace pal4
