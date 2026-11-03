#include "pal4/xml_escaper.h"

namespace pal4 {

std::string& XmlEscaper::Append(std::string& output,
                                const std::string_view input) {
    constexpr char kHex[] = "0123456789ABCDEF";
    const std::string source(input);
    for (std::size_t index = 0u; index < source.size(); ++index) {
        const auto value = static_cast<unsigned char>(source[index]);
        if (value == '&' && index + 2u < source.size() &&
            source[index + 1u] == '#' && source[index + 2u] == 'x') {
            while (index + 1u < source.size() && source[index] != ';') {
                output.push_back(source[index]);
                ++index;
            }
        }
        const auto current = static_cast<unsigned char>(source[index]);
        switch (current) {
        case '&': output.append("&amp;"); break;
        case '<': output.append("&lt;"); break;
        case '>': output.append("&gt;"); break;
        case '"': output.append("&quot;"); break;
        case '\'': output.append("&apos;"); break;
        default:
            if (current < 0x20u) {
                output.append("&#x");
                output.push_back(kHex[current >> 4u]);
                output.push_back(kHex[current & 0x0fu]);
                output.push_back(';');
            } else {
                output.push_back(static_cast<char>(current));
            }
            break;
        }
    }
    return output;
}

}  // namespace pal4
