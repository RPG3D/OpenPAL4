#include "pal4/string_character_search.h"

#include <cstring>

namespace pal4 {

std::int32_t StringCharacterSearch::FindFirstOf(
    const std::string_view text, const std::int32_t start,
    const std::string_view characters) {
    if (characters.empty() ||
        static_cast<std::uint32_t>(start) >= text.size())
        return -1;

    for (std::size_t index = static_cast<std::uint32_t>(start);
         index < text.size(); ++index) {
        if (std::memchr(characters.data(),
                        static_cast<unsigned char>(text[index]),
                        characters.size()))
            return static_cast<std::int32_t>(index);
    }
    return -1;
}

}  // namespace pal4
