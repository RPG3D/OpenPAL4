#include "pal4/xml_string_comparator.h"

#include <cctype>

namespace pal4 {

bool XmlStringComparator::HasPrefix(
    const std::uint8_t* source, const std::uint8_t* prefix,
    const bool ignore_case, [[maybe_unused]] const int encoding_mode) {
    if (source == nullptr || *source == 0u || prefix == nullptr) return false;
    while (*prefix != 0u) {
        if (*source == 0u) return false;
        const auto left = ignore_case ? std::tolower(*source) : *source;
        const auto right = ignore_case ? std::tolower(*prefix) : *prefix;
        if (left != right) return false;
        ++source;
        ++prefix;
    }
    return true;
}

}  // namespace pal4
