#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct UiOverlayRect {
    float left{};
    float top{};
    float right{};
    float bottom{};
};

class IUiTextImageOverlayRuntime {
public:
    virtual ~IUiTextImageOverlayRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void SetTextErrorFlag(std::uint32_t record,
                                  std::uint8_t flag) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCeguiString(
        std::u32string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t GetFont(
        std::uint32_t cegui_string) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::uint32_t cegui_string, std::string_view image_name) = 0;
    virtual void DestroyCeguiString(std::uint32_t cegui_string) = 0;
    virtual void CleanupTextBuffer(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FreeTextAllocation(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RenderWidth() = 0;
    [[nodiscard]] virtual std::uint32_t RenderHeight() = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::uint32_t text_data) = 0;
    [[nodiscard]] virtual float MeasureText(
        std::uint32_t font, std::string_view utf8,
        float scale) = 0;
    virtual void DrawImage(std::uint32_t image,
                           const UiOverlayRect& bounds,
                           const UiOverlayRect& clip, float alpha) = 0;
    virtual void DrawOverlayText(std::uint32_t font, std::string_view utf8,
                          const UiOverlayRect& origin,
                          const UiOverlayRect& clip, float alpha,
                          float scale) = 0;
};

class UiTextImageOverlay final {
public:
    static constexpr std::uint32_t kVtable = 0x8426c8u;

    // IDA 0x4c2270. seed is the stack-derived byte copied to object +4.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t seed,
        IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c23a0. The return value is the original scale bit pattern.
    [[nodiscard]] static std::uint32_t SetTextAndFont(
        std::uint32_t object, std::string_view text,
        std::string_view font_name, std::uint32_t scale_bits,
        IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2550 and 0x4c2570.
    [[nodiscard]] static std::uint32_t SetPosition(
        std::uint32_t object, std::uint32_t x_bits,
        std::uint32_t y_bits, IUiTextImageOverlayRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetAppearance(
        std::uint32_t object, std::uint32_t first_bits,
        std::uint32_t second_bits, IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2590. EAX after the CEGUI destructor is not a stable result.
    static void SetImage(std::uint32_t object,
                         std::string_view imageset_name,
                         std::string_view image_name,
                         std::uint32_t image_scale_bits,
                         IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c26a0. The original stack argument is unused.
    static void Render(std::uint32_t object,
                       IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2b40. Returns the final EAX value.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IUiTextImageOverlayRuntime& runtime);
};

}  // namespace pal4
