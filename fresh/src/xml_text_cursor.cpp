#include "pal4/xml_text_cursor.h"

#include <cctype>

namespace pal4 {

namespace {

std::uint32_t Utf8Step(const std::uint8_t lead) {
    if (lead < 0xc2u || lead >= 0xf5u) return 1u;
    if (lead < 0xe0u) return 2u;
    if (lead < 0xf0u) return 3u;
    return 4u;
}

}  // namespace

void XmlTextCursor::Advance(
    const std::uint32_t state, const std::uint32_t end,
    const int utf8_mode, IXmlTextCursorRuntime& runtime) {
    const auto tab_width = static_cast<std::int32_t>(
        runtime.ReadWord(state + 12u));
    if (tab_width < 1) return;
    auto cursor = runtime.ReadWord(state + 8u);
    auto line = static_cast<std::int32_t>(runtime.ReadWord(state));
    auto column = static_cast<std::int32_t>(runtime.ReadWord(state + 4u));
    while (cursor < end) {
        const auto current = runtime.ReadByte(cursor);
        if (current == 0u) return;
        if (current == '\t') {
            ++cursor;
            column = tab_width * (column / tab_width + 1);
        } else if (current == '\r' || current == '\n') {
            const auto next = runtime.ReadByte(cursor + 1u);
            ++line;
            column = 0;
            ++cursor;
            if ((current == '\r' && next == '\n') ||
                (current == '\n' && next == '\r'))
                ++cursor;
        } else if (current == 0xefu && utf8_mode == 1) {
            const auto second = runtime.ReadByte(cursor + 1u);
            if (second == 0u) return;
            const auto third = runtime.ReadByte(cursor + 2u);
            if (third == 0u) return;
            cursor += 3u;
            if (!((second == 0xbbu && third == 0xbfu) ||
                  (second == 0xbfu &&
                   (third == 0xbeu || third == 0xbfu))))
                ++column;
        } else {
            cursor += utf8_mode == 1 ? Utf8Step(current) : 1u;
            ++column;
        }
    }
    runtime.WriteWord(state, static_cast<std::uint32_t>(line));
    runtime.WriteWord(state + 4u, static_cast<std::uint32_t>(column));
    runtime.WriteWord(state + 8u, cursor);
}

const std::uint8_t* XmlTextCursor::SkipWhitespaceAndBom(
    const std::uint8_t* text, const int utf8_mode) {
    if (text == nullptr || *text == 0u) return nullptr;
    auto current = text;
    while (*current != 0u) {
        if (utf8_mode == 1 && current[0] == 0xefu &&
            ((current[1] == 0xbbu && current[2] == 0xbfu) ||
             (current[1] == 0xbfu &&
              (current[2] == 0xbeu || current[2] == 0xbfu)))) {
            current += 3u;
            continue;
        }
        if (std::isspace(*current) == 0 &&
            *current != '\n' && *current != '\r')
            break;
        ++current;
    }
    return current;
}

}  // namespace pal4
