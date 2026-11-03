#include "pal4/string_case_utilities.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace pal4 {

std::string StringCaseUtilities::UppercaseCopy(std::string_view source) {
    const auto terminator = source.find('\0');
    std::string result(source.substr(0u, terminator));
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::toupper(character));
                   });
    return result;
}

std::int32_t StringCaseUtilities::CompareInsensitive(
    const char* first, const char* second) {
    const auto left = UppercaseCopy(first != nullptr ? first : "");
    const auto right = UppercaseCopy(second != nullptr ? second : "");
    const auto common = std::min(left.size(), right.size());
    const auto comparison = std::memcmp(left.data(), right.data(), common);
    if (comparison != 0) return comparison;
    return left.size() < right.size() ? -1 :
           left.size() > right.size() ? 1 : 0;
}

}  // namespace pal4
