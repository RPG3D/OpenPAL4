#include "pal4/xml_string_parser.h"

#include "pal4/xml_entity_parser.h"
#include "pal4/xml_string_comparator.h"
#include "pal4/xml_text_cursor.h"

#include <array>
#include <cctype>
#include <cstring>

namespace pal4 {

namespace {

std::size_t ByteStep(const std::uint8_t lead, const int utf8_mode) {
    if (utf8_mode != 1 || lead < 0xc2u || lead >= 0xf5u) return 1u;
    if (lead < 0xe0u) return 2u;
    if (lead < 0xf0u) return 3u;
    return 4u;
}

bool IsWhitespace(const std::uint8_t value) {
    return value == '\r' || value == '\n' || std::isspace(value) != 0;
}

const std::uint8_t* AppendDecoded(
    const std::uint8_t* text, std::string& output, const int utf8_mode) {
    const auto step = ByteStep(*text, utf8_mode);
    if (step != 1u) {
        for (std::size_t index = 0u; index < step; ++index) {
            if (text[index] == 0u) return nullptr;
            output.push_back(static_cast<char>(text[index]));
        }
        return text + step;
    }
    if (*text != '&') {
        output.push_back(static_cast<char>(*text));
        return text + 1u;
    }
    std::array<std::uint8_t, 4> decoded{};
    std::uint32_t count = 0u;
    const auto next = XmlEntityParser::Parse(
        text, decoded.data(), count, utf8_mode);
    output.append(reinterpret_cast<const char*>(decoded.data()), count);
    return next;
}

}  // namespace

const std::uint8_t* XmlStringParser::Parse(
    const std::uint8_t* text, std::string& output,
    const bool normalize, const std::uint8_t* delimiter,
    const bool ignore_case, const int utf8_mode) const {
    output.clear();
    const bool collapse_whitespace = normalize && normalization_enabled_;
    auto current = collapse_whitespace ?
        XmlTextCursor::SkipWhitespaceAndBom(text, utf8_mode) : text;
    if (current == nullptr) return nullptr;
    const auto delimiter_length = std::strlen(
        reinterpret_cast<const char*>(delimiter));
    bool pending_space = false;
    for (;;) {
        if (*current == 0u || XmlStringComparator::HasPrefix(
                                  current, delimiter, ignore_case, utf8_mode))
            return current + delimiter_length;
        if (collapse_whitespace && IsWhitespace(*current)) {
            pending_space = true;
            ++current;
            continue;
        }
        if (pending_space) {
            output.push_back(' ');
            pending_space = false;
        }
        current = AppendDecoded(current, output, utf8_mode);
        if (current == nullptr) return nullptr;
    }
}

}  // namespace pal4
