#include "pal4/preview_text_codec.h"

#include <cstdint>
#include <stdexcept>

namespace pal4 {

std::u32string PreviewTextCodec::DecodeUtf8(std::string_view text) {
    const auto nul = text.find('\0');
    if (nul != std::string_view::npos) text = text.substr(0u, nul);
    std::u32string result;
    result.reserve(text.size());
    for (std::size_t i = 0u; i < text.size();) {
        const auto lead = static_cast<std::uint8_t>(text[i++]);
        if (lead < 0x80u) {
            result.push_back(lead);
            continue;
        }
        const auto trailing = lead < 0xe0u ? 1u : lead < 0xf0u ? 2u : 3u;
        if (text.size() - i < trailing)
            throw std::length_error("truncated UTF-8 title text");
        std::uint32_t codepoint = lead < 0xe0u ? lead & 0x1fu :
                                  lead < 0xf0u ? lead & 0x0fu : lead & 7u;
        for (std::uint32_t j = 0u; j < trailing; ++j)
            codepoint = (codepoint << 6u) |
                (static_cast<std::uint8_t>(text[i++]) & 0x3fu);
        result.push_back(codepoint);
    }
    return result;
}

}  // namespace pal4
