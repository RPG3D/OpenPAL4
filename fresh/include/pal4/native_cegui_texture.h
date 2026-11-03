#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <memory>
#include "pal4/rw_texture_dictionary.h"
#include "pal4/legacy_crc32.h"
#include "pal4/cegui_texture.h"
#include <unordered_map>

#include <windows.h>

namespace pal4 {

class NativeCeguiPackageResourceProvider;
class NativeTextureDictionaryContent;

// Native bridge for the PAL4 Renderer Texture virtual slots.
class NativeCeguiTextureBridge final {
public:
    NativeCeguiTextureBridge();
    ~NativeCeguiTextureBridge();
    NativeCeguiTextureBridge(const NativeCeguiTextureBridge&) = delete;
    NativeCeguiTextureBridge& operator=(const NativeCeguiTextureBridge&) = delete;

    [[nodiscard]] bool Bind(HMODULE cegui,
                            NativeCeguiPackageResourceProvider& resources,
                            bool own_file_system_manager = true,
                            const std::filesystem::path& fallback_directory = {});
    void Unbind() noexcept;
    [[nodiscard]] std::uintptr_t CreateFromFileSlot() const noexcept;
    [[nodiscard]] std::uintptr_t CreateFromSizeSlot() const noexcept;
    [[nodiscard]] std::uintptr_t CreateSlot() const noexcept;
    [[nodiscard]] std::uintptr_t DestroySlot() const noexcept;
    [[nodiscard]] std::uint32_t LoadCount() const noexcept { return load_count_; }
    [[nodiscard]] std::uint32_t FileDecodeCount() const noexcept {return file_decode_count_;}
    // Borrow the actual file cache used by this UI renderer, e.g. for save PNG
    // invalidation. Retire borrowers before Unbind/SDK shutdown.
    [[nodiscard]] RwTextureDictionaryState& FileTextureCache();
    [[nodiscard]] std::uint32_t CreateCount() const noexcept { return create_count_; }
    [[nodiscard]] std::uint32_t DestroyCount() const noexcept { return destroy_count_; }
    [[nodiscard]] std::uint32_t GlyphCreateCount() const noexcept {
        return glyph_create_count_;
    }
    [[nodiscard]] std::uint32_t GlyphUpdateCount() const noexcept {
        return glyph_update_count_;
    }
    [[nodiscard]] std::uint32_t Width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t Height() const noexcept { return height_; }
    [[nodiscard]] std::uint32_t VisiblePixels() const noexcept {
        return visible_pixels_;
    }
    [[nodiscard]] std::uint32_t TextureAddress() const noexcept;
    [[nodiscard]] std::uint32_t RasterAddress() const noexcept;
    [[nodiscard]] const std::string& LastError() const noexcept { return last_error_; }

private:
    static void* __fastcall CreateFromFile(void* renderer, void* unused,
                                            const void* filename,
                                            const void* group);
    static void* __fastcall CreateFromSize(void* renderer, void* unused,
                                           float size);
    static void* __fastcall Create(void* renderer, void* unused);
    static void __fastcall Destroy(void* renderer, void* unused,
                                   void* texture) noexcept;
    static unsigned short __fastcall GetWidth(void* texture) noexcept;
    static unsigned short __fastcall GetHeight(void* texture) noexcept;
    static int __fastcall HitTestAlpha(void* texture,void* unused,
                                      std::uint32_t x,std::uint32_t y);
    static void __fastcall LoadFile(void* texture, void* unused,
                                    const void* filename, const void* group);
    static void* __fastcall CreateWithDimensions(void* texture, void* unused,
                                                 const void* pixels,
                                                 unsigned int width,
                                                 unsigned int height);
    static void* __fastcall UpdateRegion(void* texture, void* unused,
                                         unsigned int x, unsigned int y,
                                         unsigned int width, unsigned int height,
                                         const void* pixels);
    static void* __fastcall DeleteObject(void* texture, void* unused,
                                         unsigned int flags) noexcept;
    void Load(void* texture, const void* filename);
    [[nodiscard]] std::uint32_t DecodeFile(const char* name);
    void UpdateFileDiagnostics(std::uint32_t texture);
    void Release(void* texture) noexcept;

    static NativeCeguiTextureBridge* active_;
    NativeCeguiPackageResourceProvider* resources_ = nullptr;
    const char* (__thiscall* string_c_str_)(const void*) = nullptr;
    std::array<std::uintptr_t, 12> vtable_{};
    std::unordered_set<void*> textures_;
    std::unordered_map<void*,CeguiTextureState> alpha_masks_;
    std::filesystem::path directory_;
    bool fs_open_ = false;
    void* file_system_ = nullptr;
    void* previous_default_file_system_ = nullptr;
    std::uint32_t load_count_ = 0;
    std::uint32_t file_decode_count_ = 0;
    RwTextureDictionaryState file_cache_{};
    LegacyCrc32 file_crc_;
    std::unique_ptr<NativeTextureDictionaryContent> file_cache_runtime_;
    std::uint32_t create_count_ = 0;
    std::uint32_t destroy_count_ = 0;
    std::uint32_t glyph_create_count_ = 0;
    std::uint32_t glyph_update_count_ = 0;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t visible_pixels_ = 0;
    std::string last_error_;
};

}  // namespace pal4
