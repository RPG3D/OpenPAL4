#pragma once
#include <windows.h>
#include <array>
#include <memory>
#include <string_view>

namespace pal4 {
// Game-owned436-byte item, with actual ListboxItem and RichTextFrame DLL owners.
// Lists borrow with auto-delete disabled; detach/reset before releasing wrapper.
class NativeRichTextListItem final {
public:
    NativeRichTextListItem(HMODULE cegui,HMODULE look,std::string_view utf8,float width);
    ~NativeRichTextListItem();
    NativeRichTextListItem(const NativeRichTextListItem&)=delete;
    NativeRichTextListItem& operator=(const NativeRichTextListItem&)=delete;
    std::uint32_t Address() const noexcept;
    void SetId(std::uint32_t);
private:
    class Runtime;
    static NativeRichTextListItem& From(void*);
    static void* __fastcall Delete(void*,void*,unsigned flags);
    static void* __fastcall Size(void*,void*,void* output);
    static void __fastcall Draw(void*,void*,void* cache,const std::array<float,4>&,
        float alpha,float modulation,const std::array<float,4>* clip);
    static void __fastcall UnusedDraw(void*,void*,std::uint32_t,std::uint32_t,std::uint32_t){}
    void Release(bool free_image);
    HMODULE module_,look_;
    std::unique_ptr<std::byte[]> image_;
    std::array<std::uint32_t,8> vtable_{};
    std::unique_ptr<Runtime> runtime_;
    bool base_ready_{};
};
}
