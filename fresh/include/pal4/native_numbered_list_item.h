#pragma once

#include "pal4/native_numbered_list_render.h"
#include <memory>
#include <string>

namespace pal4 {

// Owns a native CEGUI ListboxItem plus the game's520-byte derived image and
// modern name storage. DLL auto-delete releases the native image through host
// thunks; the wrapper can then be destroyed safely. Destroy attached lists first.
class NativeNumberedListItem final {
public:
    // Optional allocation adopts exactly520 bytes from a native save-page owner.
    NativeNumberedListItem(HMODULE module, INumberedListRenderDataRuntime& data,
        std::int32_t slot, const std::array<std::uint32_t, 13>& record,
        std::uint32_t thumbnail, bool load_mode, std::string_view name, float height,
        std::unique_ptr<std::byte[]> allocation = {});
    ~NativeNumberedListItem();
    NativeNumberedListItem(const NativeNumberedListItem&) = delete;
    NativeNumberedListItem& operator=(const NativeNumberedListItem&) = delete;
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] bool Alive() const noexcept { return constructed_; }
private:
    class ConstructionRuntime;
    static NativeNumberedListItem& From(void* self);
    static void* __fastcall Delete(void* self, void*, unsigned flags);
    static void* __fastcall GetSize(void* self, void*, void* output);
    static void __fastcall Render(void* self, void*, void* cache, const std::array<float, 4>& rect,
        float alpha, float modulation, const std::array<float, 4>* clip);
    // Original vtable+16 is4B0330: a no-op that pops three stack arguments.
    static void __fastcall UnusedDraw(void*, void*, std::uint32_t, std::uint32_t, std::uint32_t) {}
    void Release(bool free_image);
    HMODULE module_;
    NativeNumberedListRenderRuntime render_;
    std::string name_;
    std::unique_ptr<std::byte[]> image_;
    std::array<std::uint32_t, 8> vtable_{};
    bool constructed_ = false;
};
}  // namespace pal4
