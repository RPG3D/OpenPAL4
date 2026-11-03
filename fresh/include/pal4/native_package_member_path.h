#pragma once

#include <algorithm>
#include <string_view>

namespace pal4 {

// Relative CPK directory, without leading/trailing separators or dot segments.
[[nodiscard]] inline bool ValidNativePackageDirectory(std::string_view path) {
    while (!path.empty()) {
        const auto separator = path.find('\\');
        const auto segment = path.substr(0u, separator);
        if (segment.empty() || !std::all_of(segment.begin(), segment.end(),
            [](char value) {
                return (value >= '0' && value <= '9') ||
                       (value >= 'A' && value <= 'Z') ||
                       (value >= 'a' && value <= 'z') || value == '_';
            })) return false;
        if (separator == std::string_view::npos) return true;
        path.remove_prefix(separator + 1u);
    }
    return false;
}

}  // namespace pal4
