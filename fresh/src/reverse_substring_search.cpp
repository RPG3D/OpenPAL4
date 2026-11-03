#include "pal4/reverse_substring_search.h"

#include <algorithm>

namespace pal4 {

std::uint32_t ReverseSubstringSearch::Find(
    const std::uint32_t data, const std::uint32_t length,
    const std::uint32_t needle, const std::uint32_t needle_length,
    const std::uint32_t start, IReverseSubstringRuntime& runtime) {
    if (needle_length == 0u)
        return std::min(start, length);
    if (needle_length > length)
        return 0xffffffffu;

    auto position = std::min(start, length - needle_length);
    for (;;) {
        if (runtime.ReadByte(data + position) == runtime.ReadByte(needle)) {
            bool matches = true;
            for (std::uint32_t i = 1u; i < needle_length; ++i) {
                if (runtime.ReadByte(data + position + i) !=
                    runtime.ReadByte(needle + i)) {
                    matches = false;
                    break;
                }
            }
            if (matches)
                return position;
        }
        if (position == 0u)
            return 0xffffffffu;
        --position;
    }
}

}  // namespace pal4
