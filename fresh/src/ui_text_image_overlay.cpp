#include "pal4/ui_text_image_overlay.h"

#include <bit>
#include <stdexcept>
#include <string>

namespace pal4 {

std::uint32_t UiTextImageOverlay::Construct(
    const std::uint32_t object, const std::uint8_t seed,
    IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteByte(object + 4u, seed);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    runtime.WriteWord(object + 16u, 0u);
    runtime.WriteWord(object + 36u, 0x3f800000u);
    runtime.WriteWord(object + 44u, 0x3f800000u);
    runtime.WriteWord(object + 40u, 0x3f800000u);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    runtime.WriteWord(object + 48u, 0u);
    runtime.WriteWord(object + 52u, 0u);

    std::uint32_t name = 0u;
    bool name_created = false;
    try {
        name = runtime.CreateCeguiString(U"system");
        name_created = true;
        runtime.WriteWord(object + 20u, runtime.GetFont(name));
    } catch (...) {
        if (name_created) runtime.DestroyCeguiString(name);
        runtime.CleanupTextBuffer(object + 4u);
        throw;
    }
    runtime.DestroyCeguiString(name);
    return object;
}

std::uint32_t UiTextImageOverlay::SetTextAndFont(
    const std::uint32_t object, const std::string_view text,
    const std::string_view font_name, const std::uint32_t scale_bits,
    IUiTextImageOverlayRuntime& runtime) {
    const auto text_end = text.find('\0');
    const std::string_view content = text.substr(0, text_end);
    if (content.size() > 0xfffffffdu) {
        throw std::length_error("text length exceeds string buffer limit");
    }

    const std::uint32_t record = object + 4u;
    const std::uint32_t data = runtime.ReadWord(object + 8u);
    const std::uint32_t length = static_cast<std::uint32_t>(content.size());
    const std::uint8_t reference = data ? runtime.ReadByte(data - 1u) : 0u;
    if (data && reference != 0u && reference != 0xffu) {
        if (length == 0u) {
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
            runtime.SetTextErrorFlag(record, 0u);
        } else {
            runtime.ReallocateText(record, length);
        }
    } else if (length == 0u) {
        runtime.SetTextErrorFlag(record, 1u);
    } else {
        const std::uint32_t capacity = runtime.ReadWord(object + 16u);
        if (capacity > 31u || capacity < length) {
            runtime.SetTextErrorFlag(record, 1u);
            runtime.ReallocateText(record, length);
        }
    }

    if (length != 0u) {
        const std::uint32_t destination = runtime.ReadWord(object + 8u);
        runtime.CopyText(destination, content);
        runtime.WriteWord(object + 12u, length);
        runtime.WriteByte(destination + length, 0u);
    }

    const std::string_view name_bytes =
        font_name.substr(0, font_name.find('\0'));
    std::u32string name_text;
    name_text.reserve(name_bytes.size());
    for (const unsigned char byte : name_bytes) name_text.push_back(byte);
    const std::uint32_t name = runtime.CreateCeguiString(name_text);
    std::uint32_t font = 0u;
    try {
        font = runtime.GetFont(name);
    } catch (...) {
        runtime.DestroyCeguiString(name);
        throw;
    }
    runtime.WriteWord(object + 20u, font);
    runtime.DestroyCeguiString(name);
    runtime.WriteWord(object + 36u, scale_bits);
    return scale_bits;
}

std::uint32_t UiTextImageOverlay::SetPosition(
    const std::uint32_t object, const std::uint32_t x_bits,
    const std::uint32_t y_bits, IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteWord(object + 28u, x_bits);
    runtime.WriteWord(object + 32u, y_bits);
    return x_bits;
}

std::uint32_t UiTextImageOverlay::SetAppearance(
    const std::uint32_t object, const std::uint32_t first_bits,
    const std::uint32_t second_bits, IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteWord(object + 24u, first_bits);
    runtime.WriteWord(object + 40u, second_bits);
    return first_bits;
}

void UiTextImageOverlay::SetImage(
    const std::uint32_t object, const std::string_view imageset_name,
    const std::string_view image_name,
    const std::uint32_t image_scale_bits,
    IUiTextImageOverlayRuntime& runtime) {
    const std::string_view name_bytes =
        imageset_name.substr(0, imageset_name.find('\0'));
    std::u32string name_text;
    name_text.reserve(name_bytes.size());
    for (const unsigned char byte : name_bytes) name_text.push_back(byte);
    const std::uint32_t name = runtime.CreateCeguiString(name_text);
    std::uint32_t image = 0u;
    try {
        image = runtime.GetImage(name, image_name.substr(0, image_name.find('\0')));
    } catch (...) {
        runtime.DestroyCeguiString(name);
        throw;
    }
    runtime.WriteWord(object + 24u, image);
    runtime.DestroyCeguiString(name);
    runtime.WriteWord(object + 40u, image_scale_bits);
}

void UiTextImageOverlay::Render(
    const std::uint32_t object, IUiTextImageOverlayRuntime& runtime) {
    const std::uint32_t font = runtime.ReadWord(object + 20u);
    if (font == 0u) return;
    const float alpha = std::bit_cast<float>(runtime.ReadWord(object + 44u));
    if (!(alpha > 0.0f)) return;

    const float height = static_cast<float>(runtime.RenderHeight());
    const float width = static_cast<float>(runtime.RenderWidth());
    const UiOverlayRect clip{0.0f, 0.0f, width, height};
    const std::uint32_t text_data = runtime.ReadWord(object + 8u);
    const std::string first_utf8 = runtime.ConvertToUtf8(text_data);
    const float text_scale =
        std::bit_cast<float>(runtime.ReadWord(object + 36u));
    const float extent = runtime.MeasureText(font, first_utf8, text_scale);
    const float x = std::bit_cast<float>(runtime.ReadWord(object + 28u));
    const float y = std::bit_cast<float>(runtime.ReadWord(object + 32u));
    const float text_x = x - extent * 0.5f;

    const std::uint32_t image = runtime.ReadWord(object + 24u);
    if (image != 0u) {
        const float image_scale =
            std::bit_cast<float>(runtime.ReadWord(object + 40u));
        const float half_width =
            std::bit_cast<float>(runtime.ReadWord(image + 28u)) *
            image_scale * 0.5f;
        const float half_height =
            std::bit_cast<float>(runtime.ReadWord(image + 32u)) *
            image_scale * 0.5f;
        runtime.DrawImage(image,
                          {x - half_width, y - half_height,
                           x + half_width, y + half_height},
                          clip, alpha);
    }

    const std::string second_utf8 = runtime.ConvertToUtf8(text_data);
    const float font_height =
        std::bit_cast<float>(runtime.ReadWord(font + 208u));
    const float text_y = y - font_height * text_scale * 0.5f;
    runtime.DrawOverlayText(font, second_utf8,
                     {text_x, text_y, text_x, text_y},
                     clip, alpha, text_scale);
}

std::uint32_t UiTextImageOverlay::Destroy(
    const std::uint32_t object, IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    const std::uint32_t data = runtime.ReadWord(object + 8u);
    std::uint32_t result = data;
    if (data != 0u) {
        const std::uint32_t header = data - 1u;
        const std::uint8_t reference = runtime.ReadByte(header);
        result = header;
        if (reference == 0u || reference == 0xffu) {
            result = runtime.FreeTextAllocation(header);
        } else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(reference - 1u));
        }
    }
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    runtime.WriteWord(object + 16u, 0u);
    return result;
}

}  // namespace pal4
