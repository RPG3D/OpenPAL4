#pragma once
#include "pal4/ui_text_image_overlay.h"
#include <memory>
#include <windows.h>

namespace pal4 {
// Borrows the live CEGUI DLL/System. Owns host text buffers and temporary DLL
// strings; no VC6 string allocator/refcount implementation is linked or copied.
// Destroy overlays before this runtime and before stopping the UI session.
class NativeUiTextImageOverlay final : public IUiTextImageOverlayRuntime {
public:
    NativeUiTextImageOverlay(HMODULE,std::uint32_t width,std::uint32_t height);
    ~NativeUiTextImageOverlay();
    // Loads the four original static numeric fonts through the live DLL's
    // package resource provider. FontManager/UI session owns them; idempotent.
    void LoadCombatFonts();
    std::uint8_t ReadByte(std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteByte(std::uint32_t,std::uint8_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    void CopyText(std::uint32_t,std::string_view) override;
    void SetTextErrorFlag(std::uint32_t,std::uint8_t) override;
    void ReallocateText(std::uint32_t,std::uint32_t) override;
    std::uint32_t CreateCeguiString(std::u32string_view) override;
    std::uint32_t GetFont(std::uint32_t) override;
    std::uint32_t GetImage(std::uint32_t,std::string_view) override;
    void DestroyCeguiString(std::uint32_t) override;
    void CleanupTextBuffer(std::uint32_t) override;
    std::uint32_t FreeTextAllocation(std::uint32_t) override;
    std::uint32_t RenderWidth() override;
    std::uint32_t RenderHeight() override;
    std::string ConvertToUtf8(std::uint32_t) override;
    float MeasureText(std::uint32_t,std::string_view,float) override;
    void DrawImage(std::uint32_t,const UiOverlayRect&,const UiOverlayRect&,float) override;
    void DrawOverlayText(std::uint32_t,std::string_view,const UiOverlayRect&,const UiOverlayRect&,float,float) override;
    [[nodiscard]] std::size_t OwnedTexts() const noexcept;
    [[nodiscard]] std::size_t TemporaryStrings() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
